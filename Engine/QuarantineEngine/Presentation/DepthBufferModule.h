#pragma once

#ifndef DEPTH_BUFFER_MODULE_H
#define DEPTH_BUFFER_MODULE_H
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <vulkan/vulkan.h>
#include <vector>
#include <stdexcept>

#include "TextureManagerModule.h"
#include "SwapChainModule.h"
#include "CommandPoolModule.h"

class CommandPoolModule;

class DepthBufferModule : public TextureManagerModule
{
public:
    DepthBufferModule();
    void createDepthResources(VkExtent2D& swapChainExtent, QueueModule& queueModule, CommandPoolModule& commandPoolModule);
    void cleanup();
    VkFormat findDepthFormat();
private:
    bool hasStencilComponent(VkFormat format);
};

#endif
