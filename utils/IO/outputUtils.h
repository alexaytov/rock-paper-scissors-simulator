#ifndef KR_OUTPUT_UTILS_H
#define KR_OUTPUT_UTILS_H

void printSeparators(int number, char separator);
void printResultSeparators(int num);
void printTableTitles(int numberOfPlayers);
void printRowResults(int row, int numberOfPlayers, int *intermediateResults);
void printResults(int numberOfPlayers, int *intermediateResults);
void printWinners(const int *winners, int winnerValue, int currentWinnerPos);
void printFinalResults(int numberOfPlayers, int *finalResults);
int calculateSeparatorsCount(int numberOfPlayers);
void printRowTableTitles(int numberOfPlayers);

#endif //KR_OUTPUT_UTILS_H
