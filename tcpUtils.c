#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <server/utils.h>
#include "tcpUtils.h"

int initSocket(struct sockaddr_in address) {
    int socketFD;
    int opt = 1;

    if ((socketFD = socket(address.sin_family, SOCK_STREAM, 0)) == 0) {
        perror("Socket failure");
        return -1;
    }

    if (setsockopt(socketFD, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt failure");
        return -1;
    }

    if (bind(socketFD, (struct sockaddr *) &address, sizeof(address)) < 0) {
        perror("Bind failure");
        return -1;
    }

    if (listen(socketFD, 3) < 0) {
        perror("Listen error");
        return -1;
    }

    return socketFD;
}

int acceptConnection(int serverFD, struct sockaddr_in address) {
    int addrLen = sizeof(address);
    int connectionSocket;

    if ((connectionSocket = accept(serverFD, (struct sockaddr *) &address, (socklen_t *) &addrLen)) < 0) {
        perror("Accept error");
    }

    return connectionSocket;
}

void writeCharToSocket(int sockFD, char *data) { writeToSocket(sockFD, data, strlen(data)); }

void writeToSocket(int sockFD, void *data, size_t size) {
    if (send(sockFD, data, size, 0) == -1) {
        perror("There was a problem sending data to the server");
        exit(EXIT_FAILURE);
    }
}

int receiveSocketData(int sockFD, void *result) {
    setAlarm(SERVER_TIMEOUT);
    size_t readBytes = recv(sockFD, result, 100, 0);
    cancelAlarm();

    if (readBytes == -1) {
        perror("There was a problem receiving data from server");
        return 0;
    }

    if (readBytes == 0) {
        fprintf(stderr, "The connection to the socket was closed while waiting for data");
        return 0;
    }

    return 1;
}

void waitRequiredSocketResponse(int sockFD, char *requiredResponse) {
    char receiveBuffer[100] = {0};

    setAlarm(SERVER_TIMEOUT);
    if (!receiveSocketData(sockFD, receiveBuffer)) {
        exit(EXIT_FAILURE);
    }
    cancelAlarm();

    if (strcmp(receiveBuffer, requiredResponse) != 0) {
        fprintf(stderr, "An error occurred on the server: %s\n", receiveBuffer);
        exit(EXIT_FAILURE);
    }
}

int initRequiredClient(int port) {
    int sockFD = initClient(port);

    if (sockFD == -1) {
        exit(EXIT_FAILURE);
    }

    return sockFD;
}

int initClient(int port) {
    int sock;
    struct sockaddr_in serv_addr;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error\n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        return -1;
    }

    if (connect(sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection Failed");
        return -1;
    }

    return sock;
}