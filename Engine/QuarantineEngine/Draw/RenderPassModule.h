#pragma once
#ifndef RENDER_PASS_MODULE_H
#define RENDER_PASS_MODULE_H

#include "DepthBufferModule.h"
#include "AntiAliasingModule.h"
#include <QESingleton.h>

class RenderPassModule : public QESingleton<RenderPassModule>
{
private:
    friend class QESingleton<RenderPassModule>; // Permitir acceso al constructor
    DeviceModule* device_ptr;
public:
    std::shared_ptr<VkRenderPass>    renderPass;
    std::shared_ptr<VkRenderPass>    dirShadowMappingRenderPass;
    std::shared_ptr<VkRenderPass> omniShadowMappingRenderPass;

public:
    RenderPassModule();
    ~RenderPassModule();

    void cleanup();
    void createRenderPass(VkFormat swapchainFormat, VkFormat depthFormat, VkSampleCountFlagBits msaaSamples);
    void createDirShadowRenderPass(VkFormat shadowFormat);
    void createOmniShadowRenderPass(VkFormat shadowFormat, VkFormat shadowDepthFormat);
};

#endif
