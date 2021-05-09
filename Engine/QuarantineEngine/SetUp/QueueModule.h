#pragma once
#ifndef QUEUEMODULE_H
#define QUEUEMODULE_H

#include <vulkan/vulkan.h>

class QueueModule
{
public:
    VkQueue graphicsQueue;
    VkQueue presentQueue;

public:
    QueueModule() {}
};

#endif
