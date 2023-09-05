#pragma once
#include <UBO.h>

#ifndef TIMER_H
#define TIMER_H

class Timer
{
public:
    static double  DeltaTime;
    double  lastFrame;
    double  currentFrame;

private:
    static Timer* instance;

public:
    Timer();
    static Timer* getInstance();
    static void ResetInstance();
    void UpdateDeltaTime();
};

#endif // !TIMER_H
