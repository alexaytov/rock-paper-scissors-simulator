#ifndef KR_UTILS_H
#define KR_UTILS_H

#include <stdio.h>

char *logError(char *msg);

char *logExit(char *msg);

char *clearData(char *array, int len);

char *log(char *msg);

void printTimestamp(FILE *out);

void setAlarm(int time);

void cancelAlarm();

void initSignalHandler(int signal, void *handler);

#endif //KR_UTILS_H
