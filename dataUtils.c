#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include "dataUtils.h"

int buildStrToLErrorIfPresent(char *buffer, char *endPtr, int number, char **errorMessage) {
    if (buffer == endPtr) {
        *errorMessage = "Invalid number\n";
        return 1;
    }

    if (errno == ERANGE && number == LONG_MIN) {
        *errorMessage = "Underflow occurred\n";
        return 1;
    }

    if (errno == ERANGE && number == LONG_MAX) {
        *errorMessage = "Overflow occurred\n";
        return 1;
    }

    if (errno == EINVAL) {
        *errorMessage = "The base contains an unsupported value\n";
        return 1;
    }

    if (errno != 0 && number == 0) {
        *errorMessage = "Invalid number\n";
        return 1;
    }

    return 0;
}

int parseInteger(char *value, int *number, char **errMessage) {
    char *endPtr;
    errno = 0;

    *number = (int) strtol(value, &endPtr, 10);

    if (buildStrToLErrorIfPresent(value, endPtr, *number, errMessage)) {
        return 0;
    }

    return 1;
}

int getIntegerInput(char *hint, int min, int max) {
    int bufferSize = 20;

    int readInput = 0;
    char *buffer = (char *) malloc(bufferSize * sizeof(char));

    int number = -1;

    while (!readInput) {
        printf("%s", hint);

        fgets(buffer, bufferSize, stdin);

        char *endPtr;
        errno = 0;

        number = (int) strtol(buffer, &endPtr, 10);

        const char *errorMessage;
        if (buildStrToLErrorIfPresent(buffer, endPtr, number, &errorMessage))
            perror(errorMessage);
        else if (number < min || number > max)
            printf("Please enter a number between %d and %d (inclusive)\n", min, max);
        else
            readInput = 1;
    }

    free(buffer);
    return readInput;
}