#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include "serverUtils.h"

int initSocket(struct sockaddr_in address, int addrLen) {
    int socketFD;
    int opt = 1;

    if ((socketFD = socket(address.sin_family, SOCK_STREAM, 0)) == 0) {
        perror("Socket failure\n");
        return -1;
    }

    if (setsockopt(socketFD, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt failure\n");
        return -1;
    }

    if (bind(socketFD, (struct sockaddr *) &address, sizeof(address)) < 0) {
        perror("Bind failure\n");
        return -1;
    }

    if (listen(socketFD, 3) < 0) {
        perror("Listen error\n");
        return -1;
    }

    return socketFD;
}

int acceptConnection(int serverFD, struct sockaddr_in address, int addrLen) {
    int connectionSocket;

    if ((connectionSocket = accept(serverFD, (struct sockaddr *) &address, (socklen_t *) &addrLen)) < 0) {
        perror("Accept error\n");
    }

    return connectionSocket;
}

void writeCharToSocket(int sockFD, char *data) { writeToSocket(sockFD, data, strlen(data)); }

void writeToSocket(int sockFD, void *data, size_t size) {
    if (send(sockFD, data, size, 0) == -1) {
        perror("There was a problem sending data to the server\n");
        exit(EXIT_FAILURE);
    }
}

int receiveSocketData(int sockFD, char *result) {
    int readBytes = recv(sockFD, result, 100, 0);

    if (readBytes == -1) {
        perror("There was a problem receiving data from server\n");
        return 0;
    }

    if (readBytes == 0) {
        perror("The connection to the socket was closed while waiting for data\n");
        return 0;
    }

    return 1;
}

void waitRequiredSocketResponse(int sockFD, char *requiredResponse) {
    char receiveBuffer[100] = {0};

    if (!receiveSocketData(sockFD, receiveBuffer)) {
        exit(EXIT_FAILURE);
    }

    if (!strcmp(receiveBuffer, requiredResponse)) {
        fprintf(stderr, "An error occurred on the server: %s\n", receiveBuffer);
        exit(EXIT_FAILURE);
    }
}

int initClient(int port) {
    int sock = 0, valread;
    struct sockaddr_in serv_addr;
    char buffer[1024] = {0};
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error\n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported\n");
        return -1;
    }

    if (connect(sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection Failed\n");
        return -1;
    }

    return sock;
}