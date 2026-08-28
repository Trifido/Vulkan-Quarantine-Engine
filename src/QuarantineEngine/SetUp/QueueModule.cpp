#include "QueueModule.h"
#include <iostream>

QueueModule* QueueModule::instance = nullptr;
std::mutex QueueModule::instanceMutex;

QueueModule* QueueModule::getInstance()
{
    std::lock_guard<std::mutex> lock(instanceMutex);
    if (instance == NULL)
        instance = new QueueModule();

    return instance;
}

void QueueModule::ResetInstance()
{
    QueueModule* instanceToDestroy = nullptr;
    {
        std::lock_guard<std::mutex> lock(instanceMutex);
        instanceToDestroy = instance;
        instance = nullptr;
    }

    delete instanceToDestroy;
}
