#include "Timer.h"
#include <GLFW/glfw3.h>

Timer* Timer::instance = nullptr;

Timer::Timer()
{
    this->deltaTime = this->lastFrame = 0;
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
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;
}
