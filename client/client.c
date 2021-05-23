

#include <inputUtils.h>
#include <stdio.h>
#include <tcpUtils.h>
#include <communicationConstants.h>
#include <server/player.h>
#include <gameResultEvaluation.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <server/utils.h>
#include <outputUtils.h>
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

void executeCreateCommand(int sockFD, int numberOfPlayers, int iterations) {
    char *implementation = getImplementationInput();

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
    printTableTitles(numberOfPlayers);

    for (int i = 0; i < iterations; i++) {
        executeTriggerIteration(sockFD, numberOfPlayers, finalResults);
    }

    printFinalResults(numberOfPlayers, finalResults);
}

void executeTriggerIteration(int sockFD, int numberOfPlayers, int *finalResults) {
    writeCharToSocket(sockFD, TRIGGER_COMMAND);
    Choice *results = malloc(sizeof(int) * numberOfPlayers);
    receiveSocketData(sockFD, results); // TODO use method with timeout

    int *intermediateResults = calloc(numberOfPlayers, sizeof(int));
    evaluatePoints(results, numberOfPlayers, intermediateResults);
    printResultSeparators();

    printResults(numberOfPlayers, intermediateResults);
    addResultsToFinalResults(numberOfPlayers, finalResults, intermediateResults);

    free(intermediateResults);
    free(results);
}

int main() {
    initializeSignalHandlers();

    sockFD = initRequiredClient(PORT);

    int numberOfPlayers = getIntegerInput("Enter number of players: ", 1, 100);
    int iterations = getIntegerInput("Enter number of iterations: ", 1, 100);

    executeCreateCommand(sockFD, numberOfPlayers, iterations);
    executeTriggerCommand(sockFD, iterations, numberOfPlayers);

    close(sockFD);
    return 0;
}