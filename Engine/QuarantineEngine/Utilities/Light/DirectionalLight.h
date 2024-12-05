#pragma once
#ifndef DIRECTIONAL_LIGHT_H
#define DIRECTIONAL_LIGHT_H

#include "Light.h"
#include "CSMResources.h"
#include <ShaderModule.h>

class DescriptorBuffer;

class DirectionalLight : public Light
{
public:
    std::shared_ptr<CSMResources> shadowMappingResourcesPtr = nullptr;

public:
    DirectionalLight();
    DirectionalLight(std::shared_ptr<ShaderModule> shaderModule, std::shared_ptr<VkRenderPass> renderPass);
    void UpdateUniform() override;
    void CleanShadowMapResources();
};

#endif
