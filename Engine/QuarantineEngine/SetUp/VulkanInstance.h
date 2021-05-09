#pragma once
#ifndef VULKANINSTANCE_H
#define VULKANINSTANCE_H

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include "VulkanLayerAndExtension.h"

class VulkanInstance
{
private:
    VkInstance instance;
public:
    DEBUG_LEVEL debug_level;

public:
    VkResult createInstance();
    void destroyInstance();
    VkInstance& getInstance();
};

#endif
