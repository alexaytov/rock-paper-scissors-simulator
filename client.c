#include "utils.h"
#include "serverUtils.h"

#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#define PORT 8080

void InformationGatherer();

int main(int argc, char const *argv[])
{
    InformationGatherer();
}

void InformationGatherer()
{
    int sockFD = initClient(PORT);
    if (sockFD == -1)
    {
        exit(EXIT_FAILURE);
    }

    writeToSocket(sockFD, "create test 10");
    waitRequiredSocketResponse(sockFD, "ok");

    writeToSocket(sockFD, "trigger round");

    // while number of rounds
    char roundResults[100] = {0};
    receiveSocketData(sockFD, roundResults);
    // do whatever with result
}

