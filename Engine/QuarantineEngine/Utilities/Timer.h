#pragma once

#ifndef TIMER_H
#define TIMER_H

class Timer
{
public:
    double  deltaTime;
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
