#include "QueueModule.h"
#include <iostream>

QueueModule* QueueModule::instance = nullptr;

QueueModule* QueueModule::getInstance()
{
    if (instance == NULL)
        instance = new QueueModule();
    else
        std::cout << "Getting existing instance" << std::endl;

    return instance;
}
