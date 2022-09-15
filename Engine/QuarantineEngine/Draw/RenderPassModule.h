#pragma once
#ifndef RENDER_PASS_MODULE_H
#define RENDER_PASS_MODULE_H

#include "DepthBufferModule.h"
#include "AntiAliasingModule.h"

class RenderPassModule
{
private:
    DeviceModule* device_ptr;
public:
    VkRenderPass    renderPass;

public:
    RenderPassModule();
    ~RenderPassModule();
    void cleanup();
    void createRenderPass(VkFormat swapchainFormat, VkFormat depthFormat, VkSampleCountFlagBits msaaSamples);
};

#endif
