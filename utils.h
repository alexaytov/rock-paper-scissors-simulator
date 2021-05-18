#include <stdio.h>
#include <stdlib.h>

char *logError(char *msg) {
    fprintf(stderr, "%s", msg);
    return msg;
}

char *logExit(char *msg) {
    fprintf(stderr, "%s", msg);
    exit(EXIT_FAILURE);
}

char *clearData(char *array, int len) {
    memset(array, 0, len);

    return array;
}