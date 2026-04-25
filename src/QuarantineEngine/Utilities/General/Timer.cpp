#include "Timer.h"
#include <GLFW/glfw3.h>

float Timer::DeltaTime = 0.0f;
float Timer::FixedDelta = 0.0f;
float Timer::RenderAlpha = 0.0f;
uint32_t Timer::LimitFrameCounter = 0;

Timer::Timer()
{
    _frameCounter = LimitFrameCounter = 0;
    _currentFrame = 0.0f;
    DeltaTime = _accumulator = _lastFrame = 0.0f;
    FixedDelta = 1.0f / 60.0f;
}

void Timer::UpdateDeltaTime()
{
    _currentFrame = static_cast<float>(glfwGetTime());
    DeltaTime = _currentFrame - _lastFrame;
    _lastFrame = _currentFrame;

    DeltaTime = std::clamp(DeltaTime, 0.0f, 0.1f);
    _frameCounter++;
    LimitFrameCounter++;
    LimitFrameCounter &= 0xFFFFFF;
}

int Timer::ComputeFixedSteps()
{
    _accumulator += DeltaTime;
    int steps = 0;
    while (_accumulator >= FixedDelta)
    {
        steps++;
        _accumulator -= FixedDelta;
    }
    RenderAlpha = _accumulator / FixedDelta;
    return steps;
}
