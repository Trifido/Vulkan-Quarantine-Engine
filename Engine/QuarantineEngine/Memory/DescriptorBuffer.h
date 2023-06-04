#pragma once

#ifndef DESCRIPTOR_BUFFER_H
#define DESCRIPTOR_BUFFER_H

#include "DescriptorLayout.h"
#include "CustomTexture.h"
#include "UBO.h"

class DescriptorBuffer
{
private:
    DeviceModule*                   deviceModule;
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

public:
    DescriptorBuffer();
    void    createDescriptorSets(DescriptorLayout& descriptorLayout);
};

#endif // !DESCRIPTOR_BUFFER_H

