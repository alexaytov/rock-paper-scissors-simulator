#include <netinet/in.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "dataUtils.h"
#include "playerUtils.h"
#include "serverUtils.h"
#include "utils.h"

#define PORT 8080
#define SOCKET_BUFFER_SIZE 1024
#define PLAYER_BUFFER_SIZE 1024
#define ERROR_MESSAGE_SIZE 1024
#define COMMAND_DELIMITER " "

#define TRIGGER_COMMAND "trigger"
#define CREATE_COMMAND "create"

int waitClientConnection();
void initPlayers(char *implementation, int numberOfPlayers, int serverPipes[2]);
char *setupPlayerProcess(char *implementation, int numberOfPlayers, int pipes[2]);
int executeTriggerCommand(int playerPipes[2], int *currentIteration, int iterations, int *results, char *error);
char *executeCreateCommand(char *input, char *commandDelimiter, int playerPipes[2]);
void closePlayerProcess(int readerPipe, int writerPipe);

int main() {
    // setup socket and accept one connection
    int socketFD = waitClientConnection();

    // variables for player management
    int playerPipes[2] = {-1};
    int numberOfPlayers = -1;
    int currentIteration = 0;
    int iterations = -1;

    char input[SOCKET_BUFFER_SIZE] = {0};
    while (receiveSocketData(socketFD, input)) {
        char *command = strtok(input, COMMAND_DELIMITER);
        printf("Server received command %s\n", command);

        if (!strcmp(command, CREATE_COMMAND)) {
            char *implementation = strtok(NULL, COMMAND_DELIMITER);
            char *playersCountAsString = strtok(NULL, COMMAND_DELIMITER);
            char *numberOfIterationsAsString = strtok(NULL, COMMAND_DELIMITER);

            if (implementation == NULL || playersCountAsString == NULL || numberOfIterationsAsString == NULL) {
                writeCharToSocket(socketFD, logError("Required parameters for create command are missing\n"));
                continue;
            }

            char *errMessage;
            if (!parseInteger(playersCountAsString, &numberOfPlayers, errMessage)) {
                writeCharToSocket(socketFD, logError(errMessage));
                continue;
            }

            if (!parseInteger(numberOfIterationsAsString, &iterations, errMessage)) {
                writeCharToSocket(socketFD, logError(errMessage));
                continue;
            }

            if (iterations <= 0 > 100) {
                writeCharToSocket(socketFD, logError("The number of iterations can be [1:100]"));
                continue;
            }

            char *returnMessage = setupPlayerProcess(implementation, numberOfPlayers, playerPipes);
            writeCharToSocket(socketFD, returnMessage);
            continue;
        }

        if (!strcmp(command, TRIGGER_COMMAND)) {
            printf("Entered trigger command execution\n");

            int results[numberOfPlayers];
            char *error;
            if (executeTriggerCommand(playerPipes, &currentIteration, iterations, results, error)) {
                writeToSocket(socketFD, results, sizeof(results));
            } else {
                writeCharToSocket(socketFD, error);
            }
            if (currentIteration >= iterations) {
                closePlayerProcess(playerPipes[0], playerPipes[1]);
            }

            continue;
        }

        writeCharToSocket(socketFD, "Command not found");

        clearData(input, SOCKET_BUFFER_SIZE);
        printf("input: %s\n", input);
        printf("command: %s\n", command);
    }

    return 0;
}

int waitClientConnection() {
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    int socketFD = initSocket(address, addrlen);
    if (socketFD == -1) {
        exit(EXIT_FAILURE);
    }

    int connectionSocket = acceptConnection(socketFD, address, addrlen);
    if (connectionSocket == -1) {
        exit(EXIT_FAILURE);
    }

    return connectionSocket;
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

    if (procId != 0)  // child TODO replace with procId == 0
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

    return "ok";
}

int isValidNumberOfPlayers(int numberOfPlayers, char *errMessage) {
    if (numberOfPlayers <= 0) {
        errMessage = "Number of players can't be negative or zero";
        return 1;
    }

    if (numberOfPlayers > 100) {
        errMessage = "Number of players can't exceed 100";
        return 1;
    }

    return 0;
}

typedef struct CountingSemaphorePlayerData {
    int id;
    pthread_barrier_t *barrier;
    sem_t sem;
    Choice *results;
} CountingSemaphorePlayerData;

typedef struct MutexCondPlayerData {
    int id;
    pthread_barrier_t barrier;
    pthread_mutex_t mtx;
    pthread_cond_t cond;
    Choice *results;
} MutexCondPlayerData;

void *countingSemaphorePlayer(void *arg) {
    CountingSemaphorePlayerData *data = (CountingSemaphorePlayerData *)arg;

    for (;;) {
        sem_wait(&data->sem);

        data->results[data->id] = choose();

        pthread_barrier_wait(data->barrier);
    }
}

void *mutexCondPlayer(void *arg) {
    MutexCondPlayerData *data = (MutexCondPlayerData *)arg;

    for (;;) {
        pthread_mutex_lock(&data->mtx);
        pthread_cond_wait(&data->cond, &data->mtx);

        *(Choice *)arg = choose();

        pthread_mutex_unlock(&data->mtx);

        // signal that a pick has been made
    }
}

int initBarrier(pthread_barrier_t *barrier, char *errMessage, int count) {
    int error = pthread_barrier_init(barrier, NULL, count);

    if (error) {
        errMessage = logError("An error occurred when creating barrier");
    }

    return error;
}

int setupSeamphore(sem_t *sem, char *errMessage) {
    int error = sem_init(sem, 0, 0);

    if (error) {
        errMessage = logError("An error occurred when creating semaphore");
    }

    return error;
}

void closePipes(int first, int second) {
    close(first);
    close(second);
}

void exitWithError(char *errMessage, int readerPipe, int writerPipe) {
    write(writerPipe, errMessage, strlen(errMessage));

    closePipes(readerPipe, writerPipe);

    logExit(errMessage);
}

void exitWithSuccess(char *message, int readerPipe, int writerPipe) {
    write(writerPipe, message, strlen(message));
    closePipes(readerPipe, writerPipe);

    exit(EXIT_SUCCESS);
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
        if (setupSeamphore(&data->sem, semError)) {
            exitWithError(semError, serverPipes[0], serverPipes[1]);
        }

        playersData[i] = data;

        pthread_create(&playerThreads[i], NULL, countingSemaphorePlayer, (void *)data);
    }
}

void runController(int serverPipes[2], int numberOfPlayers, CountingSemaphorePlayerData **playersData,
                   pthread_barrier_t *barrier, Choice *results) {
    int readerPipe = serverPipes[0];
    int writerPipe = serverPipes[1];

    for (;;) {
        char message[1024] = {0};

        int readBytes = read(readerPipe, &message, sizeof(message));
        if (readBytes == -1) {
            exitWithError("Error reading from pipe", readerPipe, writerPipe);
        }
        if (readBytes == 0) {
            exitWithError("Pipe was closed while waiting for instruction", readerPipe, writerPipe);
        }

        if (!strcmp(message, "trigger")) {
            // trigger threads
            for (int i = 0; i < numberOfPlayers; i++) {
                sem_post(&playersData[i]->sem);
            }

            pthread_barrier_wait(barrier);

            printf("Results: ");
            for (int i = 0; i < numberOfPlayers; i++) {
                printf("%d ", results[i]);
            }
            printf("\n");
            write(writerPipe, results, sizeof(results));
            continue;
        }

        if (!strcmp(message, "end process")) {
            exitWithSuccess("ok", readerPipe, writerPipe);
        }

        char *unrecognizedCommandMessage = "Unrecognized command";
        write(writerPipe, unrecognizedCommandMessage, sizeof(unrecognizedCommandMessage));
    }
}

void countingSemaphorePlayerImplementation(int serverPipes[2], int numberOfPlayers, Choice *results) {
    int readerPipe = serverPipes[0];
    int writerPipe = serverPipes[1];

    pthread_t *playerThreads = malloc(sizeof(pthread_t) * numberOfPlayers);
    CountingSemaphorePlayerData **playersData = malloc(sizeof(CountingSemaphorePlayerData) * numberOfPlayers);

    pthread_barrier_t barrier;
    char *barrierErrMsg;
    if (initBarrier(&barrier, barrierErrMsg, numberOfPlayers + 1)) {
        exitWithError(barrierErrMsg, readerPipe, writerPipe);
    }

    setupPlayerThreads(numberOfPlayers, results, &barrier, playersData, playerThreads, serverPipes);

    runController(serverPipes, numberOfPlayers, playersData, &barrier, results);

    // cleanup
    for (int i = 0; i < numberOfPlayers; i++) {
        free(playersData[i]);
    }
    free(playersData);
    free(playerThreads);
    pthread_barrier_destroy(&barrier);
}

void initPlayers(char *implementation, int numberOfPlayers, int serverPipes[2]) {
    char *errMessage;
    if (isValidNumberOfPlayers(numberOfPlayers, errMessage)) {
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

void closePlayerProcess(int readerPipe, int writerPipe) {
    char *endProcess = "end process";
    write(writerPipe, endProcess, strlen(endProcess));

    char processResponse[PLAYER_BUFFER_SIZE];
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

    fprintf(stderr, "%s", processResponse);
}

int executeTriggerCommand(int playerPipes[2], int *currentIteration, int iterations, int *results, char *error) {
    if (playerPipes[0] == -1 || playerPipes[1] == -1) {
        error = "Please create players before triggering them";
        return 0;
    }
    int readerPipe = playerPipes[0];
    int writerPipe = playerPipes[1];

    write(writerPipe, TRIGGER_COMMAND, sizeof(TRIGGER_COMMAND));

    int readBytes = read(readerPipe, results, PLAYER_BUFFER_SIZE);

    if (readBytes == -1 || readBytes == 0) {
        error = readBytes == -1 ? "There was an error reading from the players' process"
                                : "The server -> players pipe is closed";
        return 0;
    }

    (*currentIteration)++;

    return 1;
}