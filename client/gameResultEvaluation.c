#include <server/player.h>
#include <memory.h>
#include <stdio.h>
#include "gameResultEvaluation.h"

int *buildResults(Choice winner, Choice roundResults[], int pointsPerWinner, int evaluationResults[], int size) {
    for (int i = 0; i < size; i++) {
        if (roundResults[i] == winner) {
            evaluationResults[i] = pointsPerWinner;
        }
    }

    return evaluationResults;
}

int getPointsPerWinner(int singleWinnerPrize, int playersCount) {
    return playersCount > singleWinnerPrize
           ? 1
           : singleWinnerPrize / playersCount;
}

int evaluateWinners(Choice selectedChoice, Choice enemy, Choice choices[], int size) {
    int winnersCounter = 0;

    for (int i = 0; i < size; i++) {
        // enemy found win is not possible
        if (choices[i] == enemy) {
            return 0;
        }

        // the current choice is found
        if (choices[i] == selectedChoice) {
            winnersCounter++;
        }
    }

    return winnersCounter;
}

int areAllChoicesEqual(Choice choices[], int size) {
    for (int i = 0; i < size - 1; i++) {
        if (choices[i] != choices[i + 1]) {
            return 0;
        }
    }

    return 1;
}

int *evaluatePoints(Choice choices[], int size, int *evaluationResults) {
    if (areAllChoicesEqual(choices, size)) {
        return evaluationResults;
    }

    int rockWinners = evaluateWinners(ROCK, PAPER, choices, size);
    if (rockWinners) {
        int pointPerWinner = getPointsPerWinner(WINNER_POINTS, rockWinners);
        return buildResults(ROCK, choices, pointPerWinner, evaluationResults, size);
    }

    memset(evaluationResults, 0, size);
    int paperWinners = evaluateWinners(PAPER, SCISSORS, choices, size);
    if (paperWinners) {
        int pointPerWinner = getPointsPerWinner(WINNER_POINTS, paperWinners);
        return buildResults(PAPER, choices, pointPerWinner, evaluationResults, size);
    }

    memset(evaluationResults, 0, size);
    int scissorsWinners = evaluateWinners(SCISSORS, ROCK, choices, size);
    if (scissorsWinners) {
        int pointPerWinner = getPointsPerWinner(WINNER_POINTS, scissorsWinners);
        return buildResults(SCISSORS, choices, pointPerWinner, evaluationResults, size);
    }

    return memset(evaluationResults, 0, size);;
}