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
    static RenderPassModule* instance;
    std::shared_ptr<VkRenderPass>    renderPass;
    std::shared_ptr<VkRenderPass>    dirShadowMappingRenderPass;
    std::shared_ptr<VkRenderPass> omniShadowMappingRenderPass;

public:
    RenderPassModule();
    ~RenderPassModule();

    static RenderPassModule* getInstance();
    void cleanup();
    void createRenderPass(VkFormat swapchainFormat, VkFormat depthFormat, VkSampleCountFlagBits msaaSamples);
    void createDirShadowRenderPass(VkFormat shadowFormat);
    void createOmniShadowRenderPass(VkFormat shadowFormat, VkFormat shadowDepthFormat);
};

#endif
