#include <semaphore.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "player.h"
#include "pipeUtils.h"
#include "utils.h"
#include "communicationConstants.h"

int areAllFlagsUp(const int *readyFlags, int size);

void triggerCountSemPlayerThreads(int numberOfPlayers,
                                  BinarySemaphorePlayerData **playersData,
                                  pthread_barrier_t *barrier) {
    for (int i = 0; i < numberOfPlayers; i++) {
        sem_post(&playersData[i]->sem);
    }

    pthread_barrier_wait(barrier);
}

void triggerMutexCondPlayerThreads(pthread_mutex_t *mtx,
                                   pthread_cond_t *cond,
                                   pthread_barrier_t *barrier,
                                   int *readyFlags,
                                   int numberOfPlayers) {
    int executed = 0;

    do {
        pthread_mutex_lock(mtx);

        if (!areAllFlagsUp(readyFlags, numberOfPlayers)) {
            log("Not mutex-cond players are ready to be trigger, polling...");
            pthread_mutex_unlock(mtx);
            sleep(1);
            continue;
        }

        pthread_cond_broadcast(cond);
        pthread_mutex_unlock(mtx);

        pthread_barrier_wait(barrier);
        executed = 1;
    } while (!executed);
}

int areAllFlagsUp(const int *readyFlags, int size) {
    for (int i = 0; i < size; i++) {
        if (!readyFlags[i]) {
            return 0;
        }
    }

    return 1;
}

_Noreturn void *countingSemaphorePlayer(void *arg) {
    BinarySemaphorePlayerData *data = (BinarySemaphorePlayerData *) arg;

    for (;;) {
        sem_wait(&data->sem);

        data->results[data->id] = choose();

        pthread_barrier_wait(data->barrier);
    }
}

int isSupportedImplementation(char *implementation) {
    return !strcmp(implementation, BINARY_SEM_IMPL) || !strcmp(implementation, MTX_COND_IMPL);
}

void initPlayers(char *implementation, int numberOfPlayers, int serverPipes[2]) {
    char *errMessage;
    if (isValidNumberOfPlayers(numberOfPlayers, &errMessage)) {
        logExit(errMessage);
    }

    if (!isSupportedImplementation(implementation)) {
        logExit("Unsupported implementation\n");
    }

    Choice *results = malloc(sizeof(Choice) * numberOfPlayers);

    if (!strcmp(implementation, BINARY_SEM_IMPL)) {
        binarySemPlayerImplementation(serverPipes, numberOfPlayers, results);
    }

    if (!strcmp(implementation, MTX_COND_IMPL)) {
        mutexCondPlayerImplementation(serverPipes, numberOfPlayers, results);
    }

    free(results);
}

Choice choose() {
    return (Choice) (rand() % LAST);
}

void setupMutexCondPlayerThreads(int numberOfPlayers,
                                 MutexCondPlayerData **playersData,
                                 pthread_t *playerThreads,
                                 pthread_barrier_t *barrier,
                                 Choice *results,
                                 int *readyFlags,
                                 pthread_mutex_t *mtx,
                                 pthread_cond_t *cond) {
    printTimestamp(stdout);
    printf("Creating %d mutex cond player threads\n", numberOfPlayers);

    MutexCondPlayerData *data;
    for (int i = 0; i < numberOfPlayers; i++) {
        data = malloc(sizeof(BinarySemaphorePlayerData));
        data->id = i;
        data->results = results;
        data->barrier = barrier;
        data->mtx = mtx;
        data->cond = cond;
        data->readyFlags = readyFlags;

        playersData[i] = data;

        pthread_create(&playerThreads[i], NULL, mutexCondPlayer, (void *) data);
    }
}

void mutexCondPlayerController(int serverPipes[2],
                               int numberOfPlayers,
                               pthread_mutex_t *mtx,
                               pthread_cond_t *cond,
                               pthread_barrier_t *barrier,
                               Choice *results,
                               int *readyFlags) {
    int readerPipe = serverPipes[0];
    int writerPipe = serverPipes[1];

    for (;;) {
        char message[PLAYER_BUFFER_SIZE] = {0};

        size_t readBytes = read(readerPipe, &message, sizeof(message));
        if (readBytes == -1) {
            exitWithError("Error reading from pipe", readerPipe, writerPipe);
        }
        if (readBytes == 0) {
            exitWithError("Pipe was closed while waiting for instruction", readerPipe, writerPipe);
        }

        if (!strcmp(message, TRIGGER)) {
            triggerMutexCondPlayerThreads(mtx, cond, barrier, readyFlags, numberOfPlayers);
            write(writerPipe, results, sizeof(int) * numberOfPlayers);
            continue;
        }

        if (!strcmp(message, END_PROCESS)) {
            write(writerPipe, OK, sizeof(OK));
            break;
        }

        write(writerPipe, UNRECOGNIZED_COMMAND, sizeof(UNRECOGNIZED_COMMAND));
    }
}

int initMutex(pthread_mutex_t *mtx, char **errMessage) {
    int error = pthread_mutex_init(mtx, NULL);

    if (error) {
        *errMessage = logError("An error occurred when creating mutex");
    }

    return error;
}

int initCond(pthread_cond_t *cond, char **errMessage) {
    int error = pthread_cond_init(cond, NULL);

    if (error) {
        *errMessage = logError("An error occurred when creating barrier");
    }

    return error;
}

void mutexCondPlayerImplementation(int serverPipes[2], int numberOfPlayers, Choice *results) {
    int readerPipe = serverPipes[0];
    int writerPipe = serverPipes[1];

    pthread_t *playerThreads = malloc(sizeof(pthread_t) * numberOfPlayers);
    MutexCondPlayerData **playersData = malloc(sizeof(MutexCondPlayerData) * numberOfPlayers);
    int *readyFlags = calloc(numberOfPlayers, sizeof(int));

    // init sync variables
    pthread_barrier_t barrier;
    pthread_mutex_t mtx;
    pthread_cond_t cond;
    char *errMessage;
    if (initBarrier(&barrier, &errMessage, numberOfPlayers + 1) ||
        initMutex(&mtx, &errMessage) ||
        initCond(&cond, &errMessage)) {
        exitWithError(errMessage, readerPipe, writerPipe);
    }

    setupMutexCondPlayerThreads(numberOfPlayers, playersData, playerThreads, &barrier, results, readyFlags, &mtx,
                                &cond);

    mutexCondPlayerController(serverPipes, numberOfPlayers, &mtx, &cond, &barrier, results, readyFlags);

    // cleanup
    for (int i = 0; i < numberOfPlayers; i++) {
        free(playersData[i]);
    }
    free(playersData);
    free(playerThreads);
    free(readyFlags);
    pthread_mutex_destroy(&mtx);
    pthread_cond_destroy(&cond);
    pthread_barrier_destroy(&barrier);
}

void binarySemPlayerImplementation(int *serverPipes, int numberOfPlayers, Choice *results) {
    int readerPipe = serverPipes[0];
    int writerPipe = serverPipes[1];

    pthread_t *playerThreads = malloc(sizeof(pthread_t) * numberOfPlayers);
    BinarySemaphorePlayerData **playersData = malloc(sizeof(BinarySemaphorePlayerData) * numberOfPlayers);

    pthread_barrier_t barrier;
    char *barrierErrMsg;
    if (initBarrier(&barrier, &barrierErrMsg, numberOfPlayers + 1)) {
        exitWithError(barrierErrMsg, readerPipe, writerPipe);
    }

    setupBinarySemPlayerThreads(numberOfPlayers, results, &barrier, playersData, playerThreads, serverPipes);

    binarySemPlayerController(serverPipes, numberOfPlayers, playersData, &barrier, results);

    // cleanup
    for (int i = 0; i < numberOfPlayers; i++) {
        free(playersData[i]);
    }
    free(playersData);
    free(playerThreads);
    pthread_barrier_destroy(&barrier);
}

void binarySemPlayerController(const int *serverPipes,
                               int numberOfPlayers,
                               BinarySemaphorePlayerData **playersData,
                               pthread_barrier_t *barrier,
                               Choice *results) {
    int readerPipe = serverPipes[0];
    int writerPipe = serverPipes[1];

    for (;;) {
        char message[PLAYER_BUFFER_SIZE] = {0};

        size_t readBytes = read(readerPipe, &message, sizeof(message));
        if (readBytes == -1) {
            exitWithError("Error reading from pipe", readerPipe, writerPipe);
        }
        if (readBytes == 0) {
            exitWithError("Pipe was closed while waiting for instruction", readerPipe, writerPipe);
        }

        if (!strcmp(message, TRIGGER)) {
            triggerCountSemPlayerThreads(numberOfPlayers, playersData, barrier);
            write(writerPipe, results, sizeof(int) * numberOfPlayers);
            continue;
        }

        if (!strcmp(message, END_PROCESS)) {
            write(writerPipe, OK, sizeof(OK));
            break;
        }

        write(writerPipe, UNRECOGNIZED_COMMAND, sizeof(UNRECOGNIZED_COMMAND));
    }
}

_Noreturn void *mutexCondPlayer(void *arg) {
    MutexCondPlayerData *data = (MutexCondPlayerData *) arg;

    printf("Started player with id: %d\n", data->id);

    for (;;) {
        pthread_mutex_lock(data->mtx);
        data->readyFlags[data->id] = 1;
        pthread_cond_wait(data->cond, data->mtx);

        printf("Player with id %d activated\n", data->id);
        data->results[data->id] = choose();

        pthread_mutex_unlock(data->mtx);
        pthread_barrier_wait(data->barrier);
    }
}

void closePlayerProcess(int readerPipe, int writerPipe) {
    char *endProcess = "end process";
    write(writerPipe, endProcess, strlen(endProcess));

    char processResponse[PLAYER_BUFFER_SIZE] = {0};
    size_t readBytes = read(readerPipe, processResponse, PLAYER_BUFFER_SIZE);
    if (readBytes == -1 || readBytes == 0) {
        char *error = readBytes == -1 ? "There was an error reading from the players' process\n"
                                      : "The server -> players pipe is closed\n";
        fprintf(stderr, "%s", error);
        return;
    }

    if (!strcmp(processResponse, OK)) {
        log("Successfully closed player process");
        return;
    }

    printTimestamp(stderr);
    fprintf(stderr, "%s\n%s\n", "An error occurred while closing player process", processResponse);
    exit(EXIT_FAILURE);
}

void setupBinarySemPlayerThreads(int numberOfPlayers,
                                 Choice *results,
                                 pthread_barrier_t *barrier,
                                 BinarySemaphorePlayerData **playersData,
                                 pthread_t *playerThreads,
                                 int *serverPipes) {
    printTimestamp(stdout);
    printf("Creating %d player threads\n", numberOfPlayers);

    BinarySemaphorePlayerData *data;
    for (int i = 0; i < numberOfPlayers; i++) {
        data = malloc(sizeof(BinarySemaphorePlayerData));
        data->id = i;
        data->results = results;
        data->barrier = barrier;

        char *semError;
        if (setupSemaphore(&data->sem, &semError)) {
            exitWithError(semError, serverPipes[0], serverPipes[1]);
        }

        playersData[i] = data;

        pthread_create(&playerThreads[i], NULL, countingSemaphorePlayer, (void *) data);
    }
}

int initBarrier(pthread_barrier_t *barrier, char **errMessage, int count) {
    int error = pthread_barrier_init(barrier, NULL, count);

    if (error) {
        *errMessage = logError("An error occurred when creating barrier");
    }

    return error;
}

int setupSemaphore(sem_t *sem, char **errMessage) {
    int error = sem_init(sem, 0, 0);

    if (error) {
        *errMessage = logError("An error occurred when creating semaphore");
    }

    return error;
}

int isValidNumberOfPlayers(int numberOfPlayers, char **errMessage) {
    if (numberOfPlayers <= 0) {
        *errMessage = "Number of players can't be negative or zero";
        return 1;
    }

    if (numberOfPlayers > 100) {
        *errMessage = "Number of players can't exceed 100";
        return 1;
    }

    return 0;
}