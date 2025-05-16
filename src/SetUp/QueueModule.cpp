#include "QueueModule.h"
#include <iostream>

QueueModule* QueueModule::instance = nullptr;

QueueModule* QueueModule::getInstance()
{
    if (instance == NULL)
        instance = new QueueModule();

    return instance;
}

void QueueModule::ResetInstance()
{
	delete instance;
	instance = nullptr;
}
