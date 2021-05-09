#pragma once
#ifndef FRAME_BUFFER_MODULE_H
#define FRAME_BUFFER_MODULE_H

#include <vulkan/vulkan.h>
#include <vector>
#include "DeviceModule.h"
#include "DepthBufferModule.h"

class FramebufferModule
{
private:
    DeviceModule* deviceModule;
public:
    std::vector<VkFramebuffer> swapChainFramebuffers;
    
public:
    FramebufferModule();
    void createFramebuffer(VkRenderPass& renderPass, std::vector<VkImageView>& swapChainImageViews, VkExtent2D& swapChainExtent, DepthBufferModule& depthBufferModule);
    void cleanup();
};

#endif
