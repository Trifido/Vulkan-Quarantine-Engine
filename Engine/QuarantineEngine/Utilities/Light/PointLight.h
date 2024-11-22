#pragma once
#ifndef POINT_LIGHT_H
#define POINT_LIGHT_H

#include <Light.h>
#include <OmniShadowResources.h>
#include <ShaderModule.h>

class DescriptorBuffer;

class PointLight : public Light
{
public:
    std::shared_ptr<OmniShadowResources> shadowMappingResourcesPtr = nullptr;

public:
    PointLight();
    PointLight(std::shared_ptr<ShaderModule> shaderModule, std::shared_ptr<VkRenderPass> renderPass);
    void UpdateUniform() override;
    void CleanShadowMapResources();
};

#endif
