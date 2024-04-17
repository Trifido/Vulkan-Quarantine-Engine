#pragma once

#ifndef SHADOW_MAPPING_MODULE_H
#define SHADOW_MAPPING_MODULE_H

#include <vulkan/vulkan.hpp>
#include "TextureManagerModule.h"
#include <ShadowPipelineModule.h>
#include <ShaderModule.h>

class ShadowMappingModule : public TextureManagerModule
{
public:
    uint32_t TextureSize;
    VkFormat shadowFormat;
    VkSampler depthSampler;
    std::shared_ptr<ShadowPipelineModule> shadowPipelineModule = nullptr;
    std::shared_ptr<ShaderModule> shaderModule = nullptr;
    // Depth bias (and slope) are used to avoid shadowing artifacts
    // Constant depth bias factor (always applied)
    float depthBiasConstant = 1.25f;
    // Slope depth bias factor, applied depending on polygon's slope
    float depthBiasSlope = 1.75f;

    VkFramebuffer shadowFrameBuffer;

private:
    void CreateShadowMapResources(VkRenderPass& renderPass);

public:
    ShadowMappingModule();
    ShadowMappingModule(std::shared_ptr<ShaderModule> shaderModule, VkRenderPass& renderPass);
    void cleanup() override;
};

#endif // !SHADOW_MAPPING_MODULE_H
