#ifndef KR_GAME_RESULT_EVALUATION_H
#define KR_GAME_RESULT_EVALUATION_H

#define WINNER_POINTS 2

int *buildResults(Choice winner, Choice roundResults[], int pointsPerWinner, int evaluationResults[], int size);

int getPointsPerWinner(int singleWinnerPrize, int playersCount);

int evaluateWinners(Choice selectedChoice, Choice enemy, Choice choices[], int positions[], int size);

int areAllChoicesEqual(Choice choices[], int size);

int *evaluatePoints(Choice choices[], int size, int *evaluationResults);

#endif //KR_GAME_RESULT_EVALUATION_H
