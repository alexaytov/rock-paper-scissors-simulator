#include <stdio.h>
#include <limits.h>
#include "outputUtils.h"

void printSeparators(int number, char separator) {
    for (int i = 0; i < number; i++) {
        printf("%c", separator);
    }
}

void printResultSeparators() {
    printSeparators(60, '-');
    printf("\n");
}

void printTableTitles(int numberOfPlayers) {
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

    printf("| %s: %s", currentWinnerPos ? "Winners" : "Winner", currentWinnerPos == 0 ? "no winners" : "");

    for (int i = 0; i < currentWinnerPos; i++) {
        printf("%d ", winners[i]);
    }

    printf("\n");
}