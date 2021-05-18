#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>

#include "utils.h"

#include "dataUtils.h"
#include "serverUtils.h"

#include <sys/types.h>
#include <sys/stat.h>

#include <semaphore.h>
#include <pthread.h>
#include "playerUtils.h"

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
int executeTriggerCommand(int playerPipes[2], int *results, char *error);
char *executeCreateCommand(char *input, char *commandDelimiter, int playerPipes[2]);

int main()
{
    // setup socket and accept one connection
    int socketFD = waitClientConnection();

    // variables for the player process
    int playerPipes[2] = {-1};
    int numberOfPlayers = -1;

    char input[SOCKET_BUFFER_SIZE] = {0};
    while (receiveSocketData(socketFD, input))
    {
        char *command = strtok(input, COMMAND_DELIMITER);
        printf("Server received command %s\n", command);

        if (!strcmp(command, CREATE_COMMAND))
        {
            printf("Entered create command execution\n");
            writeToSocket(socketFD, executeCreateCommand(input, COMMAND_DELIMITER, playerPipes));
            continue;
        }

        printf("playerPippes %d %d\n", playerPipes[0], playerPipes[1]);
        if (!strcmp(command, TRIGGER_COMMAND))
        {
            printf("Entered trigger command execution\n");
            printf("playerPippes %d %d\n", playerPipes[0], playerPipes[1]);
            int results[numberOfPlayers];
            char *error;
            if (executeTriggerCommand(playerPipes, results, error))
            {
                writeToSocket(socketFD, results);
            }
            else
            {
                writeToSocket(socketFD, error);
            }
            // writeToSocket(socketFD, executeTriggerCommand(playerPipes));
            continue;
        }

        writeToSocket(socketFD, "Command not found");

        clearData(input, SOCKET_BUFFER_SIZE);
        printf("input: %s\n", input);
        printf("command: %s\n", command);
    }

    return 0;
}

int waitClientConnection()
{
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    int socketFD = initSocket(address, addrlen);
    if (socketFD == -1)
    {
        exit(EXIT_FAILURE);
    }

    int connectionSocket = acceptConnection(socketFD, address, addrlen);
    if (connectionSocket == -1)
    {
        exit(EXIT_FAILURE);
    }

    return connectionSocket;
}

char *executeCreateCommand(char *input, char *commandDelimiter, int playerPipes[2])
{
    char *implementation = strtok(NULL, commandDelimiter);
    char *playersCountAsString = strtok(NULL, commandDelimiter);

    if (implementation == NULL || playersCountAsString == NULL)
    {
        return logError("Required parameters for create command are missing\n");
    }

    char *errMessage;
    int playersCount;
    if (!parseInteger(playersCountAsString, &playersCount, errMessage))
    {
        return logError(errMessage);
    }

    return setupPlayerProcess(implementation, playersCount, playerPipes);
}

char *setupPlayerProcess(char *implementation, int numberOfPlayers, int pipes[2])
{
    int playerToServerPipe[2];
    int serverToPlayerPipe[2];

    if (pipe(playerToServerPipe) || pipe(serverToPlayerPipe))
    {
        return "There was an error when creating the server <-> player pipes\n";
    }

    int procId = fork();

    if (procId == -1)
    {
        return "There was an error when creating the player process";
    }

    if (procId == 0) // child TODO replace with procId == 0
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

int isValidNumberOfPlayers(int numberOfPlayers, char *errMessage)
{
    if (numberOfPlayers <= 0)
    {
        errMessage = "Number of players can't be negative or zero";
        return 1;
    }

    if (numberOfPlayers > 100)
    {
        errMessage = "Number of players can't exceed 100";
        return 1;
    }

    return 0;
}

typedef struct CountingSemaphorePlayerData
{
    int id;
    pthread_barrier_t barrier;
    sem_t sem;
    Choice *results;
} CountingSemaphorePlayerData;

typedef struct MutexCondPlayerData
{
    int id;
    pthread_barrier_t barrier;
    pthread_mutex_t mtx;
    pthread_cond_t cond;
    Choice *results;
} MutexCondPlayerData;

void *countingSemaphorePlayer(void *arg)
{
    CountingSemaphorePlayerData *data = (CountingSemaphorePlayerData *)arg;

    for (;;)
    {
        sem_wait(&data->sem);
        data->results[data->id] = choose();

        pthread_barrier_wait(&data->barrier);
    }
}

void *mutexCondPlayer(void *arg)
{
    MutexCondPlayerData *data = (MutexCondPlayerData *)arg;

    for (;;)
    {
        pthread_mutex_lock(&data->mtx);
        pthread_cond_wait(&data->cond, &data->mtx);

        *(Choice *)arg = choose();

        pthread_mutex_unlock(&data->mtx);

        // signal that a pick has been made
    }
}

int setupBarrier(pthread_barrier_t *barrier, char *errMessage, int count)
{
    int error = pthread_barrier_init(barrier, NULL, count);

    if (error)
    {
        errMessage = logError("An error occurred when creating barrier");
    }

    return error;
}

int setupSeamphore(sem_t *sem, char *errMessage)
{
    int error = sem_init(sem, 0, 0);

    if (error)
    {
        errMessage = logError("An error occurred when creating semaphore");
    }

    return error;
}

void closePipes(int first, int second)
{
    close(first);
    close(second);
}

void exitWithError(char *errMessage, int readerPipe, int writerPipe)
{
    write(writerPipe, errMessage, sizeof(errMessage));

    closePipes(readerPipe, writerPipe);

    logExit(errMessage);
}

void initPlayers(char *implementation, int numberOfPlayers, int serverPipes[2])
{
    int readerPipe = serverPipes[0];
    int writerPipe = serverPipes[1];

    char errMessage[ERROR_MESSAGE_SIZE];
    if (isValidNumberOfPlayers(numberOfPlayers, errMessage))
    {
        logExit(errMessage);
    }

    Choice *results = malloc(sizeof(Choice) * numberOfPlayers);

    if (!strcmp(implementation, "counting-semaphore"))
    {
        pthread_t *playerThreads = malloc(sizeof(pthread_t) * numberOfPlayers);
        sem_t *playerSemaphores = malloc(sizeof(sem_t) * numberOfPlayers);

        // init barrier
        pthread_barrier_t barrier;
        char barrierErrMsg[ERROR_MESSAGE_SIZE];
        if (setupBarrier(&barrier, errMessage, numberOfPlayers + 1))
        {
            exitWithError(barrierErrMsg, readerPipe, writerPipe);
        }

        // init player threads
        for (int i = 0; i < numberOfPlayers; i++)
        {
            char *semError;
            sem_t sem;
            if (setupSeamphore(&sem, semError))
            {
                exitWithError(barrierErrMsg, readerPipe, writerPipe);
            }
            playerSemaphores[i] = sem;

            CountingSemaphorePlayerData data;
            data.id = i;
            data.results = results;
            data.sem = sem;
            data.barrier = barrier;

            pthread_create(&playerThreads[i], NULL, countingSemaphorePlayer, (void *)&data);
        }

        // controller
        for (;;)
        {
            char message[1024] = {0};

            int readBytes = read(readerPipe, &message, sizeof(message));
            if (readBytes == -1)
            {
                exitWithError("Error reading from pipe", readerPipe, writerPipe);
            }
            if (readBytes == 0)
            {
                exitWithError("Pipe was closed while waiting for instruction", readerPipe, writerPipe);
            }

            if (!strcmp(message, "trigger"))
            {
                // trigger threads
                // for (int i = 0; i < numberOfPlayers; i++)
                // {
                //     sem_post(&playerSemaphores[i]);
                // }

                // pthread_barrier_wait(&barrier);

                // write(writerPipe, results, sizeof(results));
                printf("child process received trigger instruction\n");
            }

            char *unrecognizedCommandMessage = "Unrecognized command";
            write(writerPipe, unrecognizedCommandMessage, sizeof(unrecognizedCommandMessage));
        }

        free(playerSemaphores);
        free(playerThreads);
        pthread_barrier_destroy(&barrier);
        // init number of threads
        // wait trigger in pipe
    }

    if (!strcmp(implementation, "mutex-conditional-variable"))
    {
    }

    while (1)
    {
        char buffer[PLAYER_BUFFER_SIZE] = {0};
        int readBytes = read(serverPipes[0], buffer, 1024);
        if (readBytes == -1)
        {
            logExit("There was an error reading from server pipe in child\n");
        }
        if (readBytes == 0)
        {
            logExit("The server pipe was closed\n");
        }

        if (!strcmp(buffer, "trigger"))
        {
            logExit("Player process received unsupported command\n");
        }

        // on the last iteration instead of 'ok' send 'end' so the server can reset the player pipes and close shit
        // execute trigger logic

        // send either an ok or the trigger result
    }

    // setup based on implementation
    // - mutex + cond, semaphore per thread, counting semaphore
    // send an ok for the setup
    // wait for trigger in the pipe
    // trigger and send the result in the pipe ro through shared memory

    // open named pipe for two way communication
    // get the number of players from the pipe
    // close the pipe
    // wait for trigger calls through the shared data

    free(results);
}

int executeTriggerCommand(int playerPipes[2], int *results, char *error)
{
    if (playerPipes[0] == -1 || playerPipes[1] == -1)
    {
        error = "Please create players before triggering them";
        return 0;
    }
    int readerPipe = playerPipes[0];
    int writerPipe = playerPipes[1];

    write(writerPipe, TRIGGER_COMMAND, sizeof(TRIGGER_COMMAND));

    int readBytes = read(readerPipe, results, PLAYER_BUFFER_SIZE);

    if (readBytes == -1 || readBytes == 0)
    {
        error = readBytes == -1
                    ? "There was an error reading from the players' process"
                    : "The server -> players pipe is closed";
        return 0;
    }

    return 1;
}