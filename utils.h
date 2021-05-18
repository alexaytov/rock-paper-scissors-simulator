#include <stdio.h>
#include <stdlib.h>

char *logError(char *msg)
{
    perror(msg);
    return msg;
}

char *logExit(char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}

char *clearData(char *array, int len)
{
    memset(array, 0, len);

    return array;
}