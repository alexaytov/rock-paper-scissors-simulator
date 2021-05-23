#include <stdio.h>
#include <limits.h>
#include "outputUtils.h"

void printSeparators(int number, char separator) {
    for (int i = 0; i < number; i++) {
        printf("%c", separator);
    }
}

void printResultSeparators(int num) {
    printSeparators(num, '-');
    printf("\n");
}

int calculateSeparatorsCount(int numberOfPlayers) {
    return 14 * numberOfPlayers + 21;
}

void printTableTitles(int numberOfPlayers) {
    for (int j = 0; j < numberOfPlayers; j++) {
        printf("<id>:<result> ");
    }
    printf("\n");
}

void printRowTableTitles(int numberOfPlayers) {
    printf("<row> ");
    for (int j = 0; j < numberOfPlayers; j++) {
        printf("<id>:<result> ");
    }
    printf("\n");
}

void printResults(int numberOfPlayers, int *intermediateResults) {
    int winners[numberOfPlayers - 1];
    int winnerValue = INT_MIN;
    int currentWinnerPos = 0;

    for (int j = 0; j < numberOfPlayers; j++) {
        int currentResult = intermediateResults[j];
        printf("% 4d:%-8d ", j, currentResult);

        if (currentResult >= winnerValue) {
            winnerValue = currentResult;
        }
    }

    for (int j = 0; j < numberOfPlayers; j++) {
        if (winnerValue == intermediateResults[j]) {
            winners[currentWinnerPos++] = j;
        }
    }

    printWinners(winners, winnerValue, currentWinnerPos);
}

void printRowResults(int row, int numberOfPlayers, int *intermediateResults) {
    printf("%-6d", row);
    printResults(numberOfPlayers, intermediateResults);
}

void printWinners(const int *winners, int winnerValue, int currentWinnerPos) {
    int isAtLeastOneWinnerPresent = currentWinnerPos != 0 && winnerValue != 0;

    printf("| Winner%s: ", (winnerValue != 0 && currentWinnerPos > 1) || !isAtLeastOneWinnerPresent
                           ? "s"
                           : "");
    if (!isAtLeastOneWinnerPresent) {
        printf("no winners\n");
        return;
    }

    for (int i = 0; i < currentWinnerPos; i++) {
        printf("%d ", winners[i]);
    }
    printf("\n");
}

void printFinalResults(int numberOfPlayers, int *finalResults) {
    printf("\nFinal results\n");
    printTableTitles(numberOfPlayers);
    printResultSeparators(calculateSeparatorsCount(numberOfPlayers));
    printResults(numberOfPlayers, finalResults);
}