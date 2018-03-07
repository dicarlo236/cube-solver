//
// Created by jared on 11/10/15.
//
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include "Timer.h"

void Timer::start(){
    //store time of day
    gettimeofday(&start_time, NULL);
}

double Timer::stop(){
    timeval end_time;
    long seconds, useconds;
    double duration;

    gettimeofday(&end_time, NULL);
    //calculate difference in seconds and microseconds.
    seconds = end_time.tv_sec - start_time.tv_sec;
    useconds = end_time.tv_usec - start_time.tv_usec;
    //actual duartion is sum of differences
    duration = seconds + useconds/1000000.0;

    return duration;
}

void Timer::printTime(double duration) {
    printf("%5.6f seconds\n", duration);
}
