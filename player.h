#ifndef KR_PLAYER_H
#define KR_PLAYER_H

#define TRIGGER "trigger"
#define END_PROCESS "end process"
#define PLAYER_BUFFER_SIZE 1024
#define UNRECOGNIZED_COMMAND "Unrecognized command"

#include <semaphore.h>

typedef enum Choice {
    ROCK, PAPER, SCISSORS, LAST
} Choice;

typedef struct CountingSemaphorePlayerData {
    int id;
    pthread_barrier_t *barrier;
    sem_t sem;
    Choice *results;
} CountingSemaphorePlayerData;

typedef struct MutexCondPlayerData {
    int id;
    pthread_barrier_t *barrier;
    pthread_mutex_t mtx;
    pthread_cond_t cond;
    Choice *results;
} MutexCondPlayerData;

typedef struct PlayerProcessData {
    int playerPipes[2];
    int numberOfPlayers;
    int currentIteration;
    int iterations;
} PlayerProcessData;

_Noreturn void *countingSemaphorePlayer(void *arg);

void triggerCountingSemaphorePlayerThreads(int writerPipe,
                                           int numberOfPlayers,
                                           CountingSemaphorePlayerData **playersData,
                                           pthread_barrier_t *barrier,
                                           Choice *results);

_Noreturn void countingSemaphoreController(const int serverPipes[2],
                                           int numberOfPlayers,
                                           CountingSemaphorePlayerData **playersData,
                                           pthread_barrier_t *barrier,
                                           Choice *results);


void initPlayers(char *implementation, int numberOfPlayers, int serverPipes[2]);

char *setupPlayerProcess(char *implementation, int numberOfPlayers, int pipes[2]);

void setupPlayerThreads(int numberOfPlayers,
                        Choice *results,
                        pthread_barrier_t *barrier,
                        CountingSemaphorePlayerData **playersData,
                        pthread_t *playerThreads,
                        int serverPipes[2]);

int setupSemaphore(sem_t *sem, char **errMessage);

int initBarrier(pthread_barrier_t *barrier, char **errMessage, int count);

Choice choose();

void closePlayerProcess(int readerPipe, int writerPipe);
int isValidNumberOfPlayers(int numberOfPlayers, char **errMessage);
void countingSemaphorePlayerImplementation(int *serverPipes, int numberOfPlayers, Choice *results);

#endif //KR_PLAYER_H
