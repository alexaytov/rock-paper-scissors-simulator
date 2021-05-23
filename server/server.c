#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include "tcpUtils.h"
#include "utils.h"
#include "dataUtils.h"
#include "player.h"
#include "server.h"
#include "communicationConstants.h"

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"

int sockFD;

void handleConnection(int connectionFD);

void alarmSignalHandler() {
    printf("Timeout\n");
    close(sockFD);
    exit(EXIT_FAILURE);
}

void unexpectedCloseHandler(int signal) {
    printf("Program closed with signal %d\n", signal);
    close(sockFD);
    exit(EXIT_FAILURE);
}

void initializeSignalHandlers() {
    initSignalHandler(SIGALRM, alarmSignalHandler);
    initSignalHandler(SIGINT, unexpectedCloseHandler);
    initSignalHandler(SIGTERM, unexpectedCloseHandler);
}

int main() {
    initializeSignalHandlers();

    struct sockaddr_in address;
    sockFD = setupSocket(&address);

    for (;;) {
        int connectionFD = acceptConnection(sockFD, address);
        if (connectionFD == -1) {
            close(sockFD);
            exit(EXIT_FAILURE);
        }

        handleConnection(connectionFD);
    }

    return 0;
}

void handleConnection(int connectionFD) {
    PlayerProcessData playerProcessData = {
            .playerPipes = {-1, -1},
            .iterations = 0,
            .numberOfPlayers = 0,
            .currentIteration = 0
    };

    char input[SOCKET_BUFFER_SIZE] = {0};
    while (receiveSocketData(connectionFD, input)) {
        char *command = strtok(input, COMMAND_DELIMITER);
        printf("Server received command %s\n", command);

        if (!strcmp(command, CREATE_COMMAND)) {
            char *implementation = strtok(NULL, COMMAND_DELIMITER);
            char *playersCountAsString = strtok(NULL, COMMAND_DELIMITER);
            char *numberOfIterationsAsString = strtok(NULL, COMMAND_DELIMITER);

            if (implementation == NULL || playersCountAsString == NULL || numberOfIterationsAsString == NULL) {
                writeCharToSocket(connectionFD, logError("Required parameters for create command are missing\n"));
                continue;
            }

            char *errMessage;
            if (!parseInteger(playersCountAsString, &playerProcessData.numberOfPlayers, &errMessage)) {
                writeCharToSocket(connectionFD, logError(errMessage));
                continue;
            }

            if (!parseInteger(numberOfIterationsAsString, &playerProcessData.iterations, &errMessage)) {
                writeCharToSocket(connectionFD, logError(errMessage));
                continue;
            }

            if (playerProcessData.iterations <= 0 || playerProcessData.iterations > 100) {
                writeCharToSocket(connectionFD, "The number of iterations can be [1:100]");
                continue;
            }

            char *returnMessage = setupPlayerProcess(implementation,
                                                     playerProcessData.numberOfPlayers,
                                                     playerProcessData.playerPipes);
            writeCharToSocket(connectionFD, returnMessage);
            clearData(input, SOCKET_BUFFER_SIZE);
            continue;
        }

        if (!strcmp(command, TRIGGER_COMMAND)) {
            printf("Entered trigger command execution\n");

            int results[playerProcessData.numberOfPlayers];
            char *error;
            if (executeTriggerCommand(&playerProcessData, results, &error)) {
                writeToSocket(connectionFD, results, sizeof(int) * playerProcessData.numberOfPlayers);
            } else {
                writeCharToSocket(connectionFD, error);
            }
            if (playerProcessData.currentIteration >= playerProcessData.iterations) {
                closePlayerProcess(playerProcessData.playerPipes[0], playerProcessData.playerPipes[1]);
                resetPlayerProcessData(&playerProcessData);
            }

            clearData(input, SOCKET_BUFFER_SIZE);
            continue;
        }

        writeCharToSocket(connectionFD, "Command not found");
    }
}

#pragma clang diagnostic pop

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

size_t executeTriggerCommand(PlayerProcessData *playerProcessData, int *results, char **error) {
    if (playerProcessData->playerPipes[0] == -1 || playerProcessData->playerPipes[1] == -1) {
        *error = "Please create players before triggering them";
        return 0;
    }
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

void resetPlayerProcessData(PlayerProcessData *data) {
    data->currentIteration = 0;
    data->iterations = 0;
    data->numberOfPlayers = 0;
    data->playerPipes[0] = -1;
    data->playerPipes[1] = -1;
}