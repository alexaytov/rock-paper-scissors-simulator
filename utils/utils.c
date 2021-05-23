#include <memory.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include "utils.h"

void printTimestamp(FILE *out) {
    time_t currentTime = time(NULL);
    if (currentTime == -1) {
        fprintf(stderr, "error on time func\n");
        return;
    }

    struct tm *ptm = localtime(&currentTime);
    if (ptm == NULL) {
        fprintf(stderr, "error on localtime func\n");
        return;
    }

    fprintf(out, "%d/%02d/%02d %02d:%02d:%02d ",
            ptm->tm_year + 1900, ptm->tm_mon, ptm->tm_mday,
            ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
}

char *logError(char *msg) {
    printTimestamp(stderr);
    fprintf(stderr, "%s", msg);
    return msg;
}

char *logExit(char *msg) {
    printTimestamp(stderr);
    fprintf(stderr, "%s", msg);
    exit(EXIT_FAILURE);
}

char *clearData(char *array, int len) {
    memset(array, 0, len);
    return array;
}

void setAlarm(int time) {
    if (alarm(time) == -1) {
        perror("alarm");
        exit(EXIT_FAILURE);
    }
}

void cancelAlarm() {
    alarm(0);
}

void initSignalHandler(int signal, void *handler) {
    struct sigaction act;
    sigemptyset(&act.sa_mask);
    act.sa_handler = handler;
    act.sa_flags = SA_RESTART;

    if (sigaction(signal, &act, 0) == -1) {
        perror("sigaction");
        return;
    }
}

char *log(char *msg) {
    printTimestamp(stdout);
    fprintf(stdout, "%s\n", msg);
    return msg;
}