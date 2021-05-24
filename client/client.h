#ifndef KR_CLIENT_H
#define KR_CLIENT_H

#define CREATE_COMMAND_TEMPLATE "create %s %d %d"

void executeCreateCommand(int sockFD, int numberOfPlayers, int iterations, char *implementation);

void executeTriggerIteration(int sockFD, int numberOfPlayers, int row, int *finalResults);

void timeoutHandler();

void unexpectedCloseHandler(int signal);

void initializeSignalHandlers();

void addResultsToFinalResults(int numberOfPlayers, int *finalResults, const int *intermediateResults);

void executeTriggerCommand(int sockFD, int iterations, int numberOfPlayers);

#endif //KR_CLIENT_H
