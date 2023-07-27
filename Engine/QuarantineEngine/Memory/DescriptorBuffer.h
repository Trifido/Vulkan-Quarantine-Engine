#pragma once

#ifndef DESCRIPTOR_BUFFER_H
#define DESCRIPTOR_BUFFER_H

#include "CustomTexture.h"
#include "ShaderModule.h"
#include "Camera.h"
#include "LightManager.h"
#include "UBO.h"

class DescriptorBuffer
{
private:
    DeviceModule*   deviceModule = nullptr;
    LightManager*   lightManager = nullptr;
    Camera*         camera = nullptr;

    std::shared_ptr<ShaderModule> shader = nullptr;

    VkDescriptorPool                descriptorPool;
    std::vector<VkDescriptorSet>    descriptorSets;

    std::shared_ptr<std::vector<std::shared_ptr<CustomTexture>>> textures;
    uint32_t    numUBOs = 0;
    uint32_t    numBinding = 0;
    bool        hasAnimationProperties = false;

public:
    //UBO's
    std::shared_ptr<UniformBufferObject>    cameraUBO = nullptr;
    std::shared_ptr<UniformBufferObject>    materialUBO = nullptr;
    std::shared_ptr<UniformBufferObject>    lightUBO = nullptr;
    std::shared_ptr<UniformBufferObject>    animationUBO = nullptr;

    //UNIFORM's
    std::shared_ptr<CameraUniform>          cameraUniform = nullptr;
    std::shared_ptr<MaterialUniform>        materialUniform = nullptr;
    std::shared_ptr<LightManagerUniform>    lightUniform = nullptr;
    std::shared_ptr<AnimationUniform>       animationUniform = nullptr;

private:
    void CreateDescriptorPool();
    void CheckResources();

public:
    DescriptorBuffer();
    DescriptorBuffer(std::shared_ptr<ShaderModule> shader_ptr);
    void    CreateDescriptorSets(const ShaderModule& shader);
};

#endif // !DESCRIPTOR_BUFFER_H

