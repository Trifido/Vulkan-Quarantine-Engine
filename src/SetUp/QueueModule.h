#pragma once
#ifndef QUEUEMODULE_H
#define QUEUEMODULE_H

#include <vulkan/vulkan.hpp>

class QueueModule
{
public:
    static QueueModule* instance;
    VkQueue             graphicsQueue;
    VkQueue             presentQueue;
    VkQueue             computeQueue;
public:
    static QueueModule* getInstance();
    static void ResetInstance();
};

#endif
