#pragma once

#ifndef SHADOW_MAPPING_MODULE_H
#define SHADOW_MAPPING_MODULE_H

#include <vulkan/vulkan.hpp>
#include "TextureManagerModule.h"

class ShadowMappingModule : public TextureManagerModule
{
public:
    static ShadowMappingModule* instance;
    uint32_t TextureSize;
    VkFormat shadowFormat;
    VkSampler depthSampler;
    // Depth bias (and slope) are used to avoid shadowing artifacts
    // Constant depth bias factor (always applied)
    float depthBiasConstant = 1.25f;
    // Slope depth bias factor, applied depending on polygon's slope
    float depthBiasSlope = 1.75f;

public:
    static ShadowMappingModule* getInstance();
    static void ResetInstance();
    ShadowMappingModule();
    void CreateShadowMapResources();
    void cleanup() override;
};

#endif // !SHADOW_MAPPING_MODULE_H
