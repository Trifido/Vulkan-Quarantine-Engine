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

public:
    static ShadowMappingModule* getInstance();
    static void ResetInstance();
    ShadowMappingModule();
    void CreateShadowMapResources();
    void cleanup() override;
};

#endif // !SHADOW_MAPPING_MODULE_H
