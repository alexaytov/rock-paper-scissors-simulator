#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <unistd.h>
#include <signal.h>
#include "utils.h"

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
    fprintf(stdout, "%s\n", msg);
    return msg;
}