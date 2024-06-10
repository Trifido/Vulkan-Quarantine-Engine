#pragma once
#ifndef POINT_LIGHT_H
#define POINT_LIGHT_H

#include "Light.h"
#include "ShadowMappingModule.h"
#include <ShaderModule.h>
#include <DescriptorBuffer.h>

class DescriptorBuffer;

class PointLight : public Light
{
public:
    std::shared_ptr<ShadowMappingModule> shadowMappingPtr = nullptr;
    std::shared_ptr<UniformBufferObject> shadowMapUBO = nullptr;
    std::shared_ptr<DescriptorBuffer> descriptorBuffer = nullptr;

public:
    PointLight();
    PointLight(std::shared_ptr<ShaderModule> shaderModule, VkRenderPass& renderPass);
    void UpdateUniform() override;
};

#endif
