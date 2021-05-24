#ifndef KR_SERVER_H
#define KR_SERVER_H

#include "player.h"

#define SOCKET_BUFFER_SIZE 1024
#define COMMAND_DELIMITER " "

typedef struct Result {
    int isMessage;
    char *message;
    int *results;
    size_t size;
} Result;

size_t executePlayersTriggerCommand(PlayerProcessData *playerProcessData, int *results, char **error);

void resetPlayerProcessData(PlayerProcessData *);

void handleConnection(int connectionFD);

Result triggerCreateCommand(PlayerProcessData *playerProcessData);

Result triggerTriggerCommand(PlayerProcessData *playerProcessData);

char *setupPlayerProcess(char *implementation, int numberOfPlayers, int pipes[2]);

void timeoutHandler();

void unexpectedCloseHandler(int signal);

void initializeSignalHandlers();

Result setupOutput(int *results, size_t outputSize);

Result setupCharOutput(char *message);

Result executeCommand(char *input, PlayerProcessData *playerProcessData);

int setupSocket(struct sockaddr_in *address);

#endif //KR_SERVER_H
