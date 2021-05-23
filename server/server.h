#ifndef KR_SERVER_H
#define KR_SERVER_H

#include "player.h"

#define SOCKET_BUFFER_SIZE 1024
#define COMMAND_DELIMITER " "

size_t executeTriggerCommand(PlayerProcessData *playerProcessData, int *results, char **error);

void resetPlayerProcessData(PlayerProcessData*);

int setupSocket();

char *setupPlayerProcess(char *implementation, int numberOfPlayers, int pipes[2]);

void handleConnection(int connectionFD);

#endif //KR_SERVER_H
