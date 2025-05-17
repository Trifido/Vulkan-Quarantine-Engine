#pragma once

#ifndef TIMER_H
#define TIMER_H

#include <UBO.h>
#include <QESingleton.h>

class Timer : public QESingleton<Timer>
{
private:
    friend class QESingleton<Timer>; // Permitir acceso al constructor

public:
    static float DeltaTime;
    float  lastFrame;
    float  currentFrame;
    uint32_t LimitFrameCounter;
    long long unsigned int FrameCounter;

public:
    Timer();
    void UpdateDeltaTime();
};

#endif // !TIMER_H
