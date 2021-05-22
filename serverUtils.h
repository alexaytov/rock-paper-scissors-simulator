#ifndef KR_SERVER_UTILS_H
#define KR_SERVER_UTILS_H

#include <stdio.h>

void writeToSocket(int sockFD, void *data, size_t size);
int initSocket(struct sockaddr_in address, int addrLen);
int acceptConnection(int serverFD, struct sockaddr_in address, int addrLen);
void writeCharToSocket(int sockFD, char *data);
int receiveSocketData(int sockFD, char *result);
void waitRequiredSocketResponse(int sockFD, char *requiredResponse);
int initClient(int port);

#endif //KR_SERVER_UTILS_H