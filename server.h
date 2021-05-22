#ifndef KR_SERVER_H
#define KR_SERVER_H

#define PORT 8080
#define SOCKET_BUFFER_SIZE 1024
#define COMMAND_DELIMITER " "

size_t executeTriggerCommand(PlayerProcessData playerProcessData, int *results, char **error);

void resetPlayerProcessData(PlayerProcessData);

int waitClientConnection();

#endif //KR_SERVER_H
