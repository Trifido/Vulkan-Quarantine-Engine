#pragma once

#ifndef DEPTH_BUFFER_MODULE_H
#define DEPTH_BUFFER_MODULE_H
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <vulkan/vulkan.hpp>
#include <vector>
#include <stdexcept>

#include "TextureManagerModule.h"
#include "SwapChainModule.h"
#include "AntiAliasingModule.h"

class DepthBufferModule : public TextureManagerModule
{
private:
    std::shared_ptr<AntiAliasingModule> antialiasingModule;

public:
    DepthBufferModule();
    void addAntiAliasingModule(std::shared_ptr<AntiAliasingModule> antialiasingModule);
    void createDepthResources(VkExtent2D& swapChainExtent, VkCommandPool& commandPool);
    VkFormat findDepthFormat();
private:
    bool hasStencilComponent(VkFormat format);
};

#endif
