#pragma once

#ifndef DEPTH_BUFFER_MODULE_H
#define DEPTH_BUFFER_MODULE_H
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <vulkan/vulkan.hpp>
#include <vector>
#include <stdexcept>

#include "TextureManagerModule.h"
#include "SwapChainModule.h"
#include "CommandPoolModule.h"
#include "AntiAliasingModule.h"

class CommandPoolModule;

class DepthBufferModule : public TextureManagerModule
{
private:
    AntiAliasingModule* antialiasingModule;

public:
    DepthBufferModule();
    void addAntiAliasingModule(AntiAliasingModule& antialiasingModule);
    void createDepthResources(VkExtent2D& swapChainExtent, QueueModule& queueModule, CommandPoolModule& commandPoolModule);
    VkFormat findDepthFormat();
private:
    bool hasStencilComponent(VkFormat format);
};

#endif
