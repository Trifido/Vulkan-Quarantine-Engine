#pragma once
#ifndef FRAME_BUFFER_MODULE_H
#define FRAME_BUFFER_MODULE_H

#include <vulkan/vulkan.hpp>
#include <vector>
#include "DeviceModule.h"
#include "DepthBufferModule.h"
#include "AntiAliasingModule.h"
#include "ShadowMappingModule.h"

class FramebufferModule
{
private:
    DeviceModule*       deviceModule;
    AntiAliasingModule* antialiasingModule;
    SwapChainModule*    swapchainModule;
    DepthBufferModule*  depthbufferModule;
    ShadowMappingModule* shadowMappingModule;

public:
    std::vector<VkFramebuffer> swapChainFramebuffers;
    VkFramebuffer shadowMapFramebuffer;
    
public:
    FramebufferModule();
    void createFramebuffer(VkRenderPass& renderPass);
    void createShadowFramebuffer(VkRenderPass& renderPass);
    void cleanup();
    void cleanupShadowBuffer();
};

#endif
