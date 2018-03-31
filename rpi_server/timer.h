#ifndef TIMER_H
#define TIMER_H

#include <pthread.h>
#include <stdbool.h>

typedef struct timer {
    void(*func)(void);
    bool running;
    pthread_mutex_t mutexRunning;
} Timer;

Timer* timer_init(void(*func)(void));  
void timer_start(Timer *t);
void timer_stop(Timer *t);
void timer_destroy(Timer **t);

#endif
