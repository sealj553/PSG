// c libs
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

// library headers
#include <wiringPi.h>

#include "server.h"
#include "timer.h"

#define BUTTONPIN 27
#define BUZZERPIN 17
#define STATUSPINR 2
#define STATUSPING 3
#define STATUSPINB 4
#define STATUSLED 26

#define BEEPINTERVAL 100
#define SENDHELPDELAY 3000

void on_connect();
void on_disconnect();
void on_receive(const char* str);
void sigquit();
void init();
void beep();
void take_picture();
void check_button();
void process_presses();
void condition_one();
void condition_two();
void condition_cancel();

typedef enum pressType { BUTTON_RELEASED, BUTTON_PRESSED, BUTTON_TIMEOUT } PressType;
typedef struct buttonEvent {
    PressType type;
    int time;
} ButtonEvent;

ButtonEvent lastPress = { BUTTON_RELEASED, 0 };
int press = -1;

int activation_time = -1;
bool sent_help = false;

Timer *beeper;
Timer *picture;

void on_connect(){
    printf("Connected\n");
    digitalWrite(STATUSLED, HIGH);
}

void on_disconnect(){
    printf("Disconected\n");
    digitalWrite(STATUSLED, LOW);
}

void on_receive(const char* str){
    printf("received: %s\n", str);
    server_send_text(str);
}

void sigquit(){
    printf("\naborting\n");
    digitalWrite(STATUSLED, LOW);
    digitalWrite(STATUSPINR, HIGH);
    digitalWrite(STATUSPING, HIGH);
    digitalWrite(STATUSPINB, HIGH);
    exit(0);
}

void init(){
    signal(SIGINT, sigquit);
    signal(SIGABRT, sigquit);
    signal(SIGTERM, sigquit);
    signal(SIGTSTP, sigquit); 

    wiringPiSetupGpio();
    pinMode(STATUSLED, OUTPUT);
    pinMode(STATUSPINR, OUTPUT);
    pinMode(STATUSPING, OUTPUT);
    pinMode(STATUSPINB, OUTPUT);
    pinMode(BUZZERPIN, OUTPUT);

    digitalWrite(BUZZERPIN, LOW);
    digitalWrite(STATUSLED, LOW);
    digitalWrite(STATUSPINR, HIGH);
    digitalWrite(STATUSPING, HIGH);
    digitalWrite(STATUSPINB, HIGH);

    beeper = timer_init(beep);
    picture = timer_init(take_picture);
    lastPress.time = get_time_millis();

}

void beep(){
    digitalWrite(BUZZERPIN, HIGH);
    delay(BEEPINTERVAL/2);
    digitalWrite(BUZZERPIN, LOW);
    delay(BEEPINTERVAL/2);
}

void take_picture(){
    //system/popen raspistill
    printf("snap\n"); 
    digitalWrite(STATUSPINB, LOW);
    delay(200);
    digitalWrite(STATUSPINB, HIGH);
    delay(200);
}

void check_button(){
    int delta = get_time_millis() - lastPress.time;

    if(lastPress.type == BUTTON_RELEASED){
        if(digitalRead(BUTTONPIN)){
            lastPress.type = BUTTON_PRESSED;
            lastPress.time = get_time_millis();
        }
    } else {
        if(!digitalRead(BUTTONPIN)){
            if(lastPress.type != BUTTON_TIMEOUT){
                press = delta;
            } else {
                lastPress.type = BUTTON_RELEASED;
            }
            lastPress.type = BUTTON_RELEASED;
            lastPress.time = get_time_millis();
        } else {
            if(delta > 1000){
                press = delta;
                lastPress.type = BUTTON_TIMEOUT;
            }
        }
    }
}

void condition_one(){
    timer_start(beeper);
    if(!sent_help){
        activation_time = get_time_millis();
    }
    digitalWrite(STATUSPINR, HIGH);
    digitalWrite(STATUSPING, HIGH);
    digitalWrite(STATUSPINB, LOW);
}

void condition_two(){
    timer_start(beeper);
    timer_start(picture);
    if(!sent_help){
        activation_time = get_time_millis();
    }
    digitalWrite(STATUSPINR, LOW);
    digitalWrite(STATUSPING, HIGH);
    digitalWrite(STATUSPINB, HIGH);
}

void condition_cancel(){
    timer_stop(beeper);
    timer_stop(picture);
    activation_time = -1;
    sent_help = false;
    digitalWrite(STATUSPINR, HIGH);
    digitalWrite(STATUSPING, LOW);
    digitalWrite(STATUSPINB, HIGH);
}

void process_presses(){
    // 1 press = alarm -> send help
    // 1 hold = alarm + picture every second -> send help
    // 1 long hold = cancel

    if(activation_time != -1){
        if(get_time_millis() - activation_time > SENDHELPDELAY){
            if(server_is_connected()){
                server_send_text("s");
                activation_time = -1;
                sent_help = true;
            }
        }
    }

    if(press == -1){
        return;
    }

    if(press > 20 && press <= 300){
        condition_one();
    } else if(press > 300 && press <= 1000){
        condition_two();
    } else if (press > 1000){
        condition_cancel();
    }

    press = -1;
}

int main(){
    init();
    server_init(on_connect, on_disconnect, on_receive);
    server_start();

    digitalWrite(STATUSPING, LOW);
    while(1){
        check_button();
        process_presses();
        delay(5);
    }
}
