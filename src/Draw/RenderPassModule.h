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
    std::shared_ptr<VkRenderPass>   ImGuiRenderPass;
    std::shared_ptr<VkRenderPass>   DefaultRenderPass;
    std::shared_ptr<VkRenderPass>   DirShadowMappingRenderPass;
    std::shared_ptr<VkRenderPass>   OmniShadowMappingRenderPass;

public:
    RenderPassModule();
    ~RenderPassModule();

    void cleanup();
    void CreateRenderPass(VkFormat swapchainFormat, VkFormat depthFormat, VkSampleCountFlagBits msaaSamples);
    void CreateDirShadowRenderPass(VkFormat shadowFormat);
    void CreateOmniShadowRenderPass(VkFormat shadowFormat, VkFormat shadowDepthFormat);
    void CreateImGuiRenderPass(VkFormat swapchainFormat, VkSampleCountFlagBits msaaSamples);
};

#endif
