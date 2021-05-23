#ifndef KR_OUTPUT_UTILS_H
#define KR_OUTPUT_UTILS_H

void printSeparators(int number, char separator);
void printResultSeparators();
void printTableTitles(int numberOfPlayers);
void printResults(int numberOfPlayers, int *intermediateResults);
void printWinners(const int *winners, int winnerValue, int currentWinnerPos);
void printFinalResults(int numberOfPlayers, int *finalResults);

#endif //KR_OUTPUT_UTILS_H
