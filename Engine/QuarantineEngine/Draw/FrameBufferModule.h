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
    DeviceModule*       deviceModule;
    AntiAliasingModule* antialiasingModule;
    SwapChainModule*    swapchainModule;
    DepthBufferModule*  depthbufferModule;

public:
    std::vector<VkFramebuffer> swapChainFramebuffers;
    
public:
    FramebufferModule();
    void createFramebuffer(VkRenderPass& renderPass);
    static VkFramebuffer CreateShadowFramebuffer(VkRenderPass& renderPass, VkImageView& imageView, uint32_t textureSize, VkDevice& device);
    void cleanup();
    //void cleanupShadowBuffer();
};

#endif
