#pragma once

#ifndef SHADOW_MAPPING_MODULE_H
#define SHADOW_MAPPING_MODULE_H

#include <vulkan/vulkan.hpp>
#include "TextureManagerModule.h"
#include <ShadowPipelineModule.h>
#include <ShaderModule.h>
#include <ShadowMappingMode.h>


class ShadowMappingModule : public TextureManagerModule
{
public:
    ShadowMappingMode shadowMode;
    static VkCommandPool commandPool;
    uint32_t TextureSize;
    VkFormat shadowFormat;
    VkSampler depthSampler = VK_NULL_HANDLE;
    std::array<VkImageView, 6> createCubeMapImageViews = { VK_NULL_HANDLE };
    std::shared_ptr<ShadowPipelineModule> shadowPipelineModule = nullptr;
    std::shared_ptr<ShaderModule> shaderModule = nullptr;
    // Depth bias (and slope) are used to avoid shadowing artifacts
    // Constant depth bias factor (always applied)
    float depthBiasConstant = 1.25f;
    // Slope depth bias factor, applied depending on polygon's slope
    float depthBiasSlope = 1.75f;

    VkFramebuffer shadowFrameBuffer = VK_NULL_HANDLE;
    std::array<VkFramebuffer, 6> shadowFrameBuffers = { VK_NULL_HANDLE };

    VkImage depthImage = VK_NULL_HANDLE;
    VkDeviceMemory deviceDepthMemory = VK_NULL_HANDLE;
private:
    void CreateDirectionalShadowMapResources(std::shared_ptr<VkRenderPass> renderPass);
    void CreateOmniShadowMapResources(std::shared_ptr<VkRenderPass> renderPass);
    void PrepareFramebuffers(std::shared_ptr<VkRenderPass> renderPass);

public:
    ShadowMappingModule();
    ShadowMappingModule(std::shared_ptr<ShaderModule> shaderModule, std::shared_ptr<VkRenderPass> renderPass, ShadowMappingMode mode);
    void cleanup() override;
};

#endif // !SHADOW_MAPPING_MODULE_H
