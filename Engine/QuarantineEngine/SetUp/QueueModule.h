#pragma once
#ifndef QUEUEMODULE_H
#define QUEUEMODULE_H

#include <vulkan/vulkan.hpp>

class QueueModule
{
public:
    VkQueue graphicsQueue;
    VkQueue presentQueue;
    VkQueue computeQueue;
public:
    QueueModule() {}
};

#endif
