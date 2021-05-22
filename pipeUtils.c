#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "pipeUtils.h"
#include "utils.h"

void closePipes(int first, int second) {
    close(first);
    close(second);
}

void exitWithError(char *errMessage, int readerPipe, int writerPipe) {
    write(writerPipe, errMessage, strlen(errMessage));

    closePipes(readerPipe, writerPipe);

    logError(errMessage);
}

void exitWithSuccess(char *message, int readerPipe, int writerPipe) {
    write(writerPipe, message, strlen(message));
    closePipes(readerPipe, writerPipe);

    exit(EXIT_SUCCESS);
}