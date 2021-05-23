#ifndef KR_DATA_UTILS_H
#define KR_DATA_UTILS_H

int parseInteger(char *value, int *number, char **errMessage);
int buildStrToLErrorIfPresent(char *buffer, char *endPtr, int number, char **errorMessage);

#endif //KR_DATA_UTILS_H
