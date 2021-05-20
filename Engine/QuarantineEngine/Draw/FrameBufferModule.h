#pragma once
#ifndef FRAME_BUFFER_MODULE_H
#define FRAME_BUFFER_MODULE_H

#include <vulkan/vulkan.hpp>
#include <vector>
#include "DeviceModule.h"
#include "DepthBufferModule.h"
#include "AntiAliasingModule.h"

class FramebufferModule
{
private:
    DeviceModule* deviceModule;
    AntiAliasingModule* antialias_ptr;
public:
    std::vector<VkFramebuffer> swapChainFramebuffers;
    
public:
    FramebufferModule();
    void addAntialiasingModule(AntiAliasingModule& antialiasingModule);
    void createFramebuffer(VkRenderPass& renderPass, std::vector<VkImageView>& swapChainImageViews, VkExtent2D& swapChainExtent, DepthBufferModule& depthBufferModule);
    void cleanup();
};

#endif
