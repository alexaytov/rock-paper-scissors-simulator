#include <stdio.h>
#include <server/player.h>
#include <errno.h>
#include <stdlib.h>
#include <server/dataUtils.h>
#include "inputUtils.h"

#define IMPL_INPUT_TEMPLATE "%d. Player sync: %s, Controller sync: %s"

char *getImplementationInput() {
    char firstOption[100] = {0};
    sprintf(firstOption, IMPL_INPUT_TEMPLATE, 1, "binary semaphore per player thread", "barrier");
    char secondOption[100] = {0};
    sprintf(secondOption, IMPL_INPUT_TEMPLATE, 2, "mutex and conditional variable", "barrier");

    printf("Select player implementation\n");
    printf("%s\n%s\n", firstOption, secondOption);
    int choice = getIntegerInput("Enter a option: ", 1, 2);

    if (choice == 1) {
        return COUNT_SEM_IMPL;
    }

    if (choice == 2) {
        return MTX_COND_IMPL;
    }

    // this path should never be reached
    return "Not supported implementation";
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

        char *errorMessage;
        if (buildStrToLErrorIfPresent(buffer, endPtr, number, &errorMessage))
            perror(errorMessage);
        else if (number < min || number > max)
            printf("Please enter a number between %d and %d (inclusive)\n", min, max);
        else
            readInput = 1;
    }

    free(buffer);
    return number;
}