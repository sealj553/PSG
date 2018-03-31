// c libs
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
// linux libs
#include <pthread.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/select.h>
// library headers
#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>
#include <wiringPi.h>
// project headers
#include "server.h"

#define HEARTBEAT_INTV 2000 //ms
#define BUF_SIZE 64

//private
void _pulse();
bool server_is_connected();
void _set_connected(bool b);
void _read_input();
void* _server_start();

//'globals'
static struct timeval timeout;
static fd_set readfds, writefds;
static bool connected;
static int lastMessageTime, client, maxfd;
static pthread_mutex_t mutexConnected;

static Timer *timer;

static void (*p_on_receive)(const char*);
static void (*p_on_connect)(void);
static void (*p_on_disconnect)(void);

int get_time_millis(){
    struct timeval  tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000 ;
}

bool server_is_connected(){
    pthread_mutex_lock(&mutexConnected);
    bool con = connected;
    pthread_mutex_unlock(&mutexConnected);
    return con;
}

void server_init(void(*on_connect)(void), void(*on_disconnect)(void), void(*on_receive)(const char*)){
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    timer = timer_init(_pulse);
    lastMessageTime = get_time_millis();

    pthread_mutex_init(&mutexConnected, NULL);

    //callbacks
    p_on_connect = on_connect;
    p_on_disconnect = on_disconnect;
    p_on_receive = on_receive;
}

void server_send_text(const char* str){
    FD_ZERO(&writefds);
    FD_SET(client, &writefds);

    int status = select(maxfd + 1, NULL, &writefds, 0, &timeout);

    if(status > 0 && FD_ISSET(client, &writefds)){
        printf("sent: %s\n", str);
        send(client, str, strlen(str), 0);
        send(client, "\n", 1, 0);
        lastMessageTime = get_time_millis();
    }
}

void server_start(){
    pthread_t thread;
    if(pthread_create(&thread, NULL, _server_start, NULL)){
        fprintf(stderr, "error creating thread\n");
        exit(1);
    }
    pthread_detach(thread);
}

void server_stop(){
    _set_connected(false);
    timer_stop(timer);
    close(client); //close connection
    p_on_disconnect();
}

///////////
//PRIVATE//
///////////

void* _server_start(){
    struct sockaddr_rc loc_addr = { 0 }, rem_addr = { 0 };
    unsigned int opt = sizeof(rem_addr);
    int s, sock_flags;

    while(1){
        _set_connected(false);

        // allocate socket
        s = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);

        // bind socket to port 1 of the first available bluetooth adapter
        loc_addr.rc_family = AF_BLUETOOTH;
        loc_addr.rc_bdaddr = *BDADDR_ANY;
        loc_addr.rc_channel = 1;
        bind(s, (struct sockaddr *)&loc_addr, sizeof(loc_addr));

        // put server socket into listening mode
        listen(s, 1);

        // put server socket into nonblocking mode
        sock_flags = fcntl(s, F_GETFL, 0);
        fcntl(s, F_SETFL, sock_flags | O_NONBLOCK);

        // accept one connection
        printf("Waiting for connection...\n");

        int status;
        while(1) { 
            FD_ZERO(&readfds);
            //FD_ZERO(&writefds);
            FD_SET(s, &readfds);
            maxfd = s;

            //status = select(maxfd + 1, &readfds, &writefds, 0, &timeout);
            status = select(maxfd + 1, &readfds, NULL, 0, &timeout);
            if(status > 0 && FD_ISSET(s, &readfds)){
                // incoming connection
                client = accept(s, (struct sockaddr*)&rem_addr, &opt);
                if(client >= 0){
                    break;
                }
            }
        }
        // close the server socket, leaving only the client socket open
        close(s);

        //connection information
        //char buf[24] = { 0 };
        //ba2str(&rem_addr.rc_bdaddr, buf);
        //printf("accepted connection from %s\n", buf);

        // put client socket into nonblocking mode
        sock_flags = fcntl(client, F_GETFL, 0);
        fcntl(client, F_SETFL, sock_flags | O_NONBLOCK);

        timer_start(timer);
        _set_connected(true);
        p_on_connect();

        // read data from the client
        while(server_is_connected()) {
            _read_input();
            delay(2);
        }
        server_stop();
    }
    return NULL;
}

void _read_input(){
    FD_ZERO(&readfds);
    FD_SET(client, &readfds);
    maxfd = client;

    int status = select(maxfd + 1, &readfds, NULL, 0, &timeout);
    if(status > 0 && FD_ISSET(client, &readfds)){
        // incoming data
        char buf[BUF_SIZE] = { 0 };
        memset(buf, 0, sizeof(buf));
        int bytes_read = recv(client, buf, sizeof(buf), 0);
        if(bytes_read != 0 && bytes_read != -1){
            //server_send_text(buf);
            p_on_receive(buf);
        } else {
            _set_connected(false);
        }
    }
}

void _pulse(){
    delay(HEARTBEAT_INTV);

    if(server_is_connected() && get_time_millis() - lastMessageTime > (HEARTBEAT_INTV / 2)){
        send(client, "\n", 1, 0);
    }
}

void _set_connected(bool b){
    pthread_mutex_lock(&mutexConnected);
    connected = b;
    pthread_mutex_unlock(&mutexConnected);
}
