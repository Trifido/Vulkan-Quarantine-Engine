#pragma once

#ifndef TIMER_H
#define TIMER_H

#include <UBO.h>
#include <QESingleton.h>

class Timer : public QESingleton<Timer>
{
private:
    friend class QESingleton<Timer>;

public:
    static float DeltaTime;
    static float FixedDelta;
    static uint32_t LimitFrameCounter;

private:
    float _accumulator;
    float _renderAlpha;
    float _lastFrame;
    float _currentFrame;
    long long unsigned int _frameCounter;

public:
    Timer();
    void UpdateDeltaTime();
    int ComputeFixedSteps();
    inline long long unsigned int GetFrameCount() { return _frameCounter; }
};

#endif // !TIMER_H
