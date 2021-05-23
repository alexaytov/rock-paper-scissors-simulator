

#include <utils/IO/inputUtils.h>
#include <stdio.h>
#include <utils/IO/tcpUtils.h>
#include <communicationConstants.h>
#include <server/player.h>
#include <client/gameResultEvaluation.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <utils/utils.h>
#include <utils/IO/outputUtils.h>
#include "client.h"

#define CREATE_COMMAND_TEMPLATE "create %s %d %d"

int sockFD;

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

void executeCreateCommand(int sockFD, int numberOfPlayers, int iterations, char *implementation) {
    char createCommand[100] = {0};
    sprintf(createCommand, CREATE_COMMAND_TEMPLATE, implementation, numberOfPlayers, iterations);
    writeCharToSocket(sockFD, createCommand);

    waitRequiredSocketResponse(sockFD, OK);
}

void addResultsToFinalResults(int numberOfPlayers, int *finalResults, const int *intermediateResults) {
    for (int j = 0; j < numberOfPlayers; j++) {
        finalResults[j] += finalResults[j] + intermediateResults[j];
    }
}

void executeTriggerCommand(int sockFD, int iterations, int numberOfPlayers) {
    int *finalResults = calloc(numberOfPlayers, sizeof(int));
    printf("\nIntermediate results\n");
    printRowTableTitles(numberOfPlayers);

    for (int i = 0; i < iterations; i++) {
        executeTriggerIteration(sockFD, numberOfPlayers, i, finalResults);
    }

    printFinalResults(numberOfPlayers, finalResults);
}

void executeTriggerIteration(int sockFD, int numberOfPlayers, int row, int *finalResults) {
    writeCharToSocket(sockFD, TRIGGER_COMMAND);
    Choice *results = malloc(sizeof(int) * numberOfPlayers);

    waitRequiredSocketResponse(sockFD, OK); // TODO use method with timeout
    writeCharToSocket(sockFD, OK);
    receiveSocketData(sockFD, results); // TODO use method with timeout

    int *intermediateResults = calloc(numberOfPlayers, sizeof(int));
    evaluatePoints(results, numberOfPlayers, intermediateResults);
    printResultSeparators(calculateSeparatorsCount(numberOfPlayers));

    printRowResults(row, numberOfPlayers, intermediateResults);
    addResultsToFinalResults(numberOfPlayers, finalResults, intermediateResults);

    free(intermediateResults);
    free(results);
}

int main() {
    initializeSignalHandlers();

    // get user input
    int numberOfPlayers = getIntegerInput("Enter number of players: ", 1, 100);
    int iterations = getIntegerInput("Enter number of iterations: ", 1, 100);
    char *implementation = getImplementationInput();

    sockFD = initRequiredClient(PORT);
    log("Initialized client<->server connection");

    executeCreateCommand(sockFD, numberOfPlayers, iterations, implementation);
    log("Executed create command");

    executeTriggerCommand(sockFD, iterations, numberOfPlayers);

    close(sockFD);
    return 0;
}