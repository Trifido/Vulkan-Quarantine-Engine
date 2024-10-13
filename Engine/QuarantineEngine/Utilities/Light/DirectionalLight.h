#pragma once
#ifndef DIRECTIONAL_LIGHT_H
#define DIRECTIONAL_LIGHT_H

#include "Light.h"
#include "ShadowMappingModule.h"
#include <ShaderModule.h>
#include <DescriptorBuffer.h>

class DescriptorBuffer;

class DirectionalLight : public Light
{
public:
    std::shared_ptr<ShadowMappingModule> shadowMappingPtr = nullptr;

    std::shared_ptr<UniformBufferObject> shadowMapUBO = nullptr;
    std::shared_ptr<DescriptorBuffer> descriptorBuffer = nullptr;

public:
    DirectionalLight();
    DirectionalLight(std::shared_ptr<ShaderModule> shaderModule, std::shared_ptr<VkRenderPass> renderPass);
    void UpdateUniform() override;
    void CleanShadowMapResources();
};

#endif
