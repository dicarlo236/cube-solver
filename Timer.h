//
// Created by jared on 11/10/15.
//

#ifndef CUBEWIZARD_TIMER_H
#define CUBEWIZARD_TIMER_H



class Timer {
private:
    timeval start_time;

public:
    //Start the timer
    void start();
    //Stop the timer and return time since start in milliseconds.
    double stop();
    //print a duration
    static void printTime(double duration);

};


#endif //CUBEWIZARD_TIMER_H
