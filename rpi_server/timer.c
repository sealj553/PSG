#include <stdio.h>
#include <stdlib.h>

#include "timer.h"

//private
void* _timer_run(void *ptr);
bool _is_running(Timer *t);

//////////
//public//
//////////

Timer* timer_init(void(*func)(void)){
    Timer *t = malloc(sizeof(Timer));
    if(t == NULL){
        fprintf(stderr, "Unable to allocate memory\n");
        exit(1);
    }
    t->func = func;
    t->running = false;
    pthread_mutex_init(&(t->mutexRunning), NULL);

    return t;
}

void timer_start(Timer *t){
    if(t->running){
        return;
    }

    t->running = true;

    pthread_t thread;

    if(pthread_create(&thread, NULL, _timer_run, t)) {
        fprintf(stderr, "error creating thread\n");
        exit(1);
    }
    pthread_detach(thread);
}

void timer_stop(Timer *t){
    pthread_mutex_lock(&(t->mutexRunning));
    t->running = false;
    pthread_mutex_unlock(&(t->mutexRunning));
}

void timer_destroy(Timer **t){
    free(*t);
    *t = NULL;
}

///////////
//private//
///////////

void* _timer_run(void *ptr){
    Timer *t = (Timer*)ptr;

    while(_is_running(t)){
        t->func();
    }
    return NULL;
}

bool _is_running(Timer *t){
    pthread_mutex_lock(&(t->mutexRunning));
    bool running = t->running;
    pthread_mutex_unlock(&(t->mutexRunning));
    return running; 
}
