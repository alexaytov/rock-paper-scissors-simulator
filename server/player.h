#ifndef KR_PLAYER_H
#define KR_PLAYER_H

#define TRIGGER "trigger"
#define END_PROCESS "end process"
#define PLAYER_BUFFER_SIZE 1024
#define UNRECOGNIZED_COMMAND "Unrecognized command"

#define BINARY_SEM_IMPL "binary-sem"
#define MTX_COND_IMPL "mtx-cond"

#include <semaphore.h>

typedef enum Choice {
    ROCK, PAPER, SCISSORS, LAST
} Choice;

typedef struct CountingSemaphorePlayerData {
    int id;
    pthread_barrier_t *barrier;
    sem_t sem;
    Choice *results;
} BinarySemaphorePlayerData;

typedef struct MutexCondPlayerData {
    int id;
    pthread_barrier_t *barrier;
    pthread_mutex_t *mtx;
    pthread_cond_t *cond;
    int *readyFlags;
    Choice *results;
} MutexCondPlayerData;

typedef struct PlayerProcessData {
    int playerPipes[2];
    int numberOfPlayers;
    int currentIteration;
    int iterations;
} PlayerProcessData;

_Noreturn void *binarySemaphorePlayer(void *arg);

void triggerBinarySemPlayerThreads(int numberOfPlayers,
                                   BinarySemaphorePlayerData **playersData,
                                   pthread_barrier_t *barrier);

void binarySemPlayerController(const int serverPipes[2],
                               int numberOfPlayers,
                               BinarySemaphorePlayerData **playersData,
                               pthread_barrier_t *barrier,
                               Choice *results);

_Noreturn void *mutexCondPlayer(void *arg);

void initPlayers(char *implementation, int numberOfPlayers, int serverPipes[2]);

void setupBinarySemPlayerThreads(int numberOfPlayers,
                                 Choice *results,
                                 pthread_barrier_t *barrier,
                                 BinarySemaphorePlayerData **playersData,
                                 pthread_t *playerThreads,
                                 int *serverPipes);

int initSemaphore(sem_t *sem, char **errMessage);

int initBarrier(pthread_barrier_t *barrier, char **errMessage, int count);

Choice choose();

void closePlayerProcess(int readerPipe, int writerPipe);

int isValidNumberOfPlayers(int numberOfPlayers, char **errMessage);

void binarySemPlayerImplementation(int *serverPipes, int numberOfPlayers, Choice *results);

void mutexCondPlayerImplementation(int serverPipes[2], int numberOfPlayers, Choice *results);

void mutexCondPlayerController(int serverPipes[2],
                               int numberOfPlayers,
                               pthread_mutex_t *mtx,
                               pthread_cond_t *cond,
                               pthread_barrier_t *barrier,
                               Choice *results,
                               int *readyFlags);

int isSupportedImplementation(char *implementation);

void setupMutexCondPlayerThreads(int numberOfPlayers,
                                 MutexCondPlayerData **playersData,
                                 pthread_t *playerThreads,
                                 pthread_barrier_t *barrier,
                                 Choice *results,
                                 int *readyFlags,
                                 pthread_mutex_t *mtx,
                                 pthread_cond_t *cond);

int initCond(pthread_cond_t *cond, char **errMessage);

int initMutex(pthread_mutex_t *mtx, char **errMessage);

void triggerMutexCondPlayerThreads(pthread_mutex_t *mtx,
                                   pthread_cond_t *cond,
                                   pthread_barrier_t *barrier,
                                   int *readyFlags,
                                   int numberOfPlayers);

#endif //KR_PLAYER_H
