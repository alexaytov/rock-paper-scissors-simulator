#include <stdlib.h>
#include <time.h>

#define NUMBER_OF_OPTIONS 3

typedef enum Choice
{
    ROCK,
    PAPER,
    SCISSORS
} Choice;

Choice choose();

Choice choose()
{
    return (Choice)(rand() & NUMBER_OF_OPTIONS);
}