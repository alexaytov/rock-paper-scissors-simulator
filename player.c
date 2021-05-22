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

void triggerCountingSemaphorePlayerThreads(int writerPipe,
                                           int numberOfPlayers,
                                           CountingSemaphorePlayerData **playersData,
                                           pthread_barrier_t *barrier,
                                           Choice *results) {
    for (int i = 0; i < numberOfPlayers; i++) {
        sem_post(&playersData[i]->sem);
    }

    pthread_barrier_wait(barrier);

    write(writerPipe, results, sizeof(int) * numberOfPlayers);
}

_Noreturn void *countingSemaphorePlayer(void *arg) {
    CountingSemaphorePlayerData *data = (CountingSemaphorePlayerData *) arg;

    for (;;) {
        sem_wait(&data->sem);

        data->results[data->id] = choose();

        pthread_barrier_wait(data->barrier);
    }
}

void initPlayers(char *implementation, int numberOfPlayers, int serverPipes[2]) {
    char *errMessage;
    if (isValidNumberOfPlayers(numberOfPlayers, &errMessage)) {
        logExit(errMessage);
    }

    Choice *results = malloc(sizeof(Choice) * numberOfPlayers);

    if (!strcmp(implementation, "counting-semaphore")) {
        countingSemaphorePlayerImplementation(serverPipes, numberOfPlayers, results);
    }

    if (!strcmp(implementation, "mutex-conditional-variable")) {
    }

    free(results);
}

Choice choose() {
    return (Choice) (rand() % LAST);
}

char *setupPlayerProcess(char *implementation, int numberOfPlayers, int pipes[2]) {
    int playerToServerPipe[2];
    int serverToPlayerPipe[2];

    if (pipe(playerToServerPipe) || pipe(serverToPlayerPipe)) {
        return "There was an error when creating the server <-> player pipes\n";
    }

    int procId = fork();

    if (procId == -1) {
        return "There was an error when creating the player process";
    }

    if (procId == 0)  // child TODO replace with procId == 0
    {
        close(serverToPlayerPipe[1]);
        close(playerToServerPipe[0]);

        int childPipes[2] = {serverToPlayerPipe[0], playerToServerPipe[1]};

        initPlayers(implementation, numberOfPlayers, childPipes);

        exit(EXIT_SUCCESS);
    }

    // parent

    close(serverToPlayerPipe[0]);
    close(playerToServerPipe[1]);

    // int *serverPipes = {playerToServerPipe[0], serverToPlayerPipe[1]};
    pipes[0] = playerToServerPipe[0];
    pipes[1] = serverToPlayerPipe[1];

    return OK;
}

void countingSemaphorePlayerImplementation(int *serverPipes, int numberOfPlayers, Choice *results) {
    int readerPipe = serverPipes[0];
    int writerPipe = serverPipes[1];

    pthread_t *playerThreads = malloc(sizeof(pthread_t) * numberOfPlayers);
    CountingSemaphorePlayerData **playersData = malloc(sizeof(CountingSemaphorePlayerData) * numberOfPlayers);

    pthread_barrier_t barrier;
    char *barrierErrMsg;
    if (initBarrier(&barrier, &barrierErrMsg, numberOfPlayers + 1)) {
        exitWithError(barrierErrMsg, readerPipe, writerPipe);
    }

    setupPlayerThreads(numberOfPlayers, results, &barrier, playersData, playerThreads, serverPipes);

    countingSemaphoreController(serverPipes, numberOfPlayers, playersData, &barrier, results);

    // cleanup
    for (int i = 0; i < numberOfPlayers; i++) {
        free(playersData[i]);
    }
    free(playersData);
    free(playerThreads);
    pthread_barrier_destroy(&barrier);
}

void countingSemaphoreController(const int *serverPipes, int numberOfPlayers, CountingSemaphorePlayerData **playersData,
                                 pthread_barrier_t *barrier, Choice *results) {
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
            triggerCountingSemaphorePlayerThreads(writerPipe, numberOfPlayers, playersData, barrier, results);
            continue;
        }

        if (!strcmp(message, END_PROCESS)) {
            exitWithSuccess(OK, readerPipe, writerPipe);
        }

        write(writerPipe, UNRECOGNIZED_COMMAND, sizeof(UNRECOGNIZED_COMMAND));
    }
}

_Noreturn void *mutexCondPlayer(void *arg) {
    MutexCondPlayerData *data = (MutexCondPlayerData *) arg;

    for (;;) {
        pthread_mutex_lock(&data->mtx);
        pthread_cond_wait(&data->cond, &data->mtx);

        *(Choice *) arg = choose();

        pthread_mutex_unlock(&data->mtx);

        pthread_barrier_wait(data->barrier);
    }
}

void closePlayerProcess(int readerPipe, int writerPipe) {
    char *endProcess = "end process";
    write(writerPipe, endProcess, strlen(endProcess));

    char processResponse[PLAYER_BUFFER_SIZE] = {0};
    int readBytes = read(readerPipe, processResponse, PLAYER_BUFFER_SIZE);
    if (readBytes == -1 || readBytes == 0) {
        char *error = readBytes == -1 ? "There was an error reading from the players' process\n"
                                      : "The server -> players pipe is closed\n";
        fprintf(stderr, "%s", error);
        return;
    }

    if (!strcmp(processResponse, "ok")) {
        printf("Successfully closed player process\n");
        return;
    }

    fprintf(stderr, "%s\n%s\n", "An error occurred while closing player process", processResponse);
    exit(EXIT_FAILURE);
}

void setupPlayerThreads(int numberOfPlayers, Choice *results, pthread_barrier_t *barrier,
                        CountingSemaphorePlayerData **playersData, pthread_t *playerThreads, int serverPipes[2]) {
    printf("Creating %d player threads\n", numberOfPlayers);

    CountingSemaphorePlayerData *data;
    for (int i = 0; i < numberOfPlayers; i++) {
        data = malloc(sizeof(CountingSemaphorePlayerData));
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