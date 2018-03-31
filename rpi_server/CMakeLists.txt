cmake_minimum_required(VERSION 3.6)

project(psg)
set(CMAKE_C_FLAGS "-std=c99 -Wall -Wextra -pthread -lbluetooth -lwiringPi")

add_executable(psg main.c timer.c server.c)
