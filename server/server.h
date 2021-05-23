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

size_t executeTriggerCommand(PlayerProcessData *playerProcessData, int *results, char **error);

void resetPlayerProcessData(PlayerProcessData *);

void handleConnection(int connectionFD);

Result triggerCreateCommand(PlayerProcessData *playerProcessData);

Result triggerTriggerCommand(PlayerProcessData *playerProcessData);

int setupSocket();

char *setupPlayerProcess(char *implementation, int numberOfPlayers, int pipes[2]);

void handleConnection(int connectionFD);

#endif //KR_SERVER_H
