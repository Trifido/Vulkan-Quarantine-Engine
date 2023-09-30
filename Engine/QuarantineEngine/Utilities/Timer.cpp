#include "Timer.h"
#include <GLFW/glfw3.h>

Timer* Timer::instance = nullptr;
float Timer::DeltaTime = 0;

Timer::Timer()
{
    this->DeltaTime = this->lastFrame = 0;
    this->FrameCounter = this->LimitFrameCounter = 0;
}

Timer* Timer::getInstance()
{
    if (instance == nullptr)
    {
        instance = new Timer();
    }
    return instance;
}

void Timer::ResetInstance()
{
    delete instance;
    instance = nullptr;
}

void Timer::UpdateDeltaTime()
{
    currentFrame = glfwGetTime();
    DeltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;
    FrameCounter++;
    LimitFrameCounter++;
    LimitFrameCounter &= 0xFFFFFF; // Limita a 24 bits (16,777,215 frames)
}
