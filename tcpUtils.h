#ifndef KR_SERVER_UTILS_H
#define KR_SERVER_UTILS_H

#include <stdio.h>

#define SERVER_TIMEOUT 10

void writeToSocket(int sockFD, void *data, size_t size);
int initSocket(struct sockaddr_in address);
int acceptConnection(int serverFD, struct sockaddr_in address);
void writeCharToSocket(int sockFD, char *data);
int receiveSocketData(int sockFD, void *result);
void waitRequiredSocketResponse(int sockFD, char *requiredResponse);
int initClient(int port);
int initRequiredClient(int port);

#endif //KR_SERVER_UTILS_H