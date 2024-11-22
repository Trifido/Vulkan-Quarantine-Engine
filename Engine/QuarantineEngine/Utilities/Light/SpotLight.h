#pragma once
#ifndef SPOT_LIGHT_H
#define SPOT_LIGHT_H

#include "Light.h"
#include "OmniShadowResources.h"
#include <ShaderModule.h>
#include <DescriptorBuffer.h>

class DescriptorBuffer;

class SpotLight : public Light
{
public:
    //std::shared_ptr<OmniShadowResources> shadowMappingPtr = nullptr;

    std::shared_ptr<UniformBufferObject> shadowMapUBO = nullptr;
    //std::shared_ptr<DescriptorBuffer> descriptorBuffer = nullptr;

public:
    SpotLight();
    SpotLight(std::shared_ptr<ShaderModule> shaderModule, std::shared_ptr<VkRenderPass> renderPass);
    void UpdateUniform() override;
};

#endif
