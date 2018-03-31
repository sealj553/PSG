#ifndef SERVER_H
#define SERVER_H

//c
#include <stdbool.h>

//linux
#include <sys/time.h>

//project
#include "timer.h"

void server_init();
void server_start();
void server_stop();
void server_send_text(const char* str);
int get_time_millis();
bool server_is_connected();

#endif
