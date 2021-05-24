#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include "utils/IO/tcpUtils.h"
#include "utils/utils.h"
#include "utils/dataUtils.h"
#include "player.h"
#include "server.h"
#include "communicationConstants.h"

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"

int sockFD;

void timeoutHandler() {
    printTimestamp(stdout);
    printf("Timeout\n");
    close(sockFD);
    exit(EXIT_FAILURE);
}

void unexpectedCloseHandler(int signal) {
    printTimestamp(stdout);
    printf("Program closed with signal %d\n", signal);
    close(sockFD);
    exit(EXIT_FAILURE);
}

void initializeSignalHandlers() {
    initSignalHandler(SIGALRM, timeoutHandler);
    initSignalHandler(SIGINT, unexpectedCloseHandler);
    initSignalHandler(SIGTERM, unexpectedCloseHandler);
}

int main() {
    initializeSignalHandlers();

    logInfo("Setting up socket");
    struct sockaddr_in address;
    sockFD = setupSocket(&address);
    logInfo("Socket is setup for listening");

    while (1) {
        int connectionFD = acceptConnection(sockFD, address);
        if (connectionFD == -1) {
            close(sockFD);
            exit(EXIT_FAILURE);
        }
        logInfo("Server accepted connection");

        handleConnection(connectionFD);
    }

    return 0;
}

Result setupOutput(int *results, size_t outputSize) {
    Result result;
    result.size = outputSize;
    result.results = results;
    result.isMessage = 0;

    return result;
}

Result setupCharOutput(char *message) {
    Result result;

    result.isMessage = 1;
    result.size = strlen(message);
    result.message = message;

    return result;
}

Result executeCommand(char *input, PlayerProcessData *playerProcessData) {
    char *command = strtok(input, COMMAND_DELIMITER);
    printTimestamp(stdout);
    printf("Server received command %s\n", command);

    if (!strcmp(command, CREATE_COMMAND)) {
        return triggerCreateCommand(playerProcessData);
    }

    if (!strcmp(command, TRIGGER_COMMAND)) {
        return triggerTriggerCommand(playerProcessData);
    }

    return setupCharOutput(logError("Command not found"));
}

Result triggerTriggerCommand(PlayerProcessData *playerProcessData) {
    logInfo("Executing trigger command");

    if (playerProcessData->playerPipes[0] == -1 || playerProcessData->playerPipes[1] == -1) {
        return setupCharOutput(logError("Please create players before triggering them"));
    }

    int *results = calloc(playerProcessData->numberOfPlayers, sizeof(int));
    char *error;
    size_t isExecutionSuccessful = executePlayersTriggerCommand(playerProcessData, results, &error);

    Result result;

    if (isExecutionSuccessful) {
        result = setupOutput(results, sizeof(int) * playerProcessData->numberOfPlayers);
    } else {
        result = setupCharOutput(error);
    }

    if (playerProcessData->currentIteration >= playerProcessData->iterations) {
        closePlayerProcess(playerProcessData->playerPipes[0], playerProcessData->playerPipes[1]);
        resetPlayerProcessData(playerProcessData);
    }

    return result;
}

Result triggerCreateCommand(PlayerProcessData *playerProcessData) {
    logInfo("Executing create command");
    char *implementation = strtok(NULL, COMMAND_DELIMITER);
    char *playersCountAsString = strtok(NULL, COMMAND_DELIMITER);
    char *numberOfIterationsAsString = strtok(NULL, COMMAND_DELIMITER);

    if (implementation == NULL || playersCountAsString == NULL || numberOfIterationsAsString == NULL) {
        return setupCharOutput(logError("Required parameters for create command are missing\n"));
    }

    char *errMessage;
    if (!parseInteger(playersCountAsString, &playerProcessData->numberOfPlayers, &errMessage) ||
        !parseInteger(numberOfIterationsAsString, &playerProcessData->iterations, &errMessage)) {
        return setupCharOutput(logError(errMessage));
    }

    if (playerProcessData->iterations <= 0 || playerProcessData->iterations > 100) {
        return setupCharOutput(logError("The number of iterations can be [1:100]"));
    }

    char *playerProcessSetupResult = setupPlayerProcess(implementation,
                                                        playerProcessData->numberOfPlayers,
                                                        playerProcessData->playerPipes);
    return setupCharOutput(playerProcessSetupResult);
}

void handleConnection(int connectionFD) {
    PlayerProcessData playerProcessData = {
            .playerPipes = {-1, -1},
            .iterations = 0,
            .numberOfPlayers = 0,
            .currentIteration = 0
    };

    char operation[SOCKET_BUFFER_SIZE] = {0};
    while (receiveSocketData(connectionFD, operation)) {
        Result result = executeCommand(operation, &playerProcessData);

        if (result.isMessage) {
            writeToSocket(connectionFD, result.message, result.size);
        } else {
            writeCharToSocket(connectionFD, OK);
            waitRequiredSocketResponse(connectionFD, OK);
            writeToSocket(connectionFD, result.results, result.size);
        }
        logInfo("Returned result");

        clearData(operation, SOCKET_BUFFER_SIZE);
    }
}

int setupSocket(struct sockaddr_in *address) {
    address->sin_family = AF_INET;
    address->sin_addr.s_addr = INADDR_ANY;
    address->sin_port = htons(PORT);

    int socketFD = initSocket(*address);
    if (socketFD == -1) {
        exit(EXIT_FAILURE);
    }

    return socketFD;
}

size_t executePlayersTriggerCommand(PlayerProcessData *playerProcessData, int *results, char **error) {
    int readerPipe = playerProcessData->playerPipes[0];
    int writerPipe = playerProcessData->playerPipes[1];

    write(writerPipe, TRIGGER_COMMAND, sizeof(TRIGGER_COMMAND));

    size_t readBytes = read(readerPipe, results, PLAYER_BUFFER_SIZE);

    if (readBytes == -1 || readBytes == 0) {
        *error = readBytes == -1 ? "There was an error reading from the players' process"
                                 : "The server -> players pipe is closed";
        return 0;
    }

    playerProcessData->currentIteration++;
    return readBytes;
}

char *setupPlayerProcess(char *implementation, int numberOfPlayers, int pipes[2]) {
    int playerToServerPipe[2];
    int serverToPlayerPipe[2];

    if (pipe(playerToServerPipe) || pipe(serverToPlayerPipe)) {
        return "There was an error when creating the server <-> player pipes";
    }

    int procId = fork();

    if (procId == -1) {
        return "There was an error when creating the player process";
    }

    if (procId == 0) {
        close(serverToPlayerPipe[1]);
        close(playerToServerPipe[0]);

        int childPipes[2] = {serverToPlayerPipe[0], playerToServerPipe[1]};

        initPlayers(implementation, numberOfPlayers, childPipes);

        exit(EXIT_SUCCESS);
    }

    // parent
    close(serverToPlayerPipe[0]);
    close(playerToServerPipe[1]);

    pipes[0] = playerToServerPipe[0];
    pipes[1] = serverToPlayerPipe[1];

    return OK;
}

void resetPlayerProcessData(PlayerProcessData *data) {
    data->currentIteration = 0;
    data->iterations = 0;
    data->numberOfPlayers = 0;
    data->playerPipes[0] = -1;
    data->playerPipes[1] = -1;
}