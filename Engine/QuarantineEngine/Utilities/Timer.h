#pragma once
#include <UBO.h>

#ifndef TIMER_H
#define TIMER_H

class Timer
{
public:
    static float DeltaTime;
    float  lastFrame;
    float  currentFrame;
    uint32_t LimitFrameCounter;
    long long unsigned int FrameCounter;
private:
    static Timer* instance;

public:
    Timer();
    static Timer* getInstance();
    static void ResetInstance();
    void UpdateDeltaTime();
};

#endif // !TIMER_H
