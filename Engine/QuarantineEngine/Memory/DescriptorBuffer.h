#pragma once

#ifndef DESCRIPTOR_BUFFER_H
#define DESCRIPTOR_BUFFER_H

#include "CustomTexture.h"
#include "ShaderModule.h"
#include "Camera.h"
#include "LightManager.h"
#include "UBO.h"
#include <MaterialData.h>

class DescriptorBuffer
{
private:
    DeviceModule*   deviceModule = nullptr;
    LightManager*   lightManager = nullptr;
    Camera*         camera = nullptr;

    VkDescriptorPool                descriptorPool;
    std::vector<VkDescriptorSet>    descriptorSets;

    std::shared_ptr<std::vector<std::shared_ptr<CustomTexture>>> textures;
    uint32_t    numUBOs = 0;
    uint32_t    numBinding = 0;
    bool        hasAnimationProperties = false;

public:
    //UBO's
    std::shared_ptr<UniformBufferObject>    materialUBO = nullptr;
    std::shared_ptr<UniformBufferObject>    animationUBO = nullptr;

    //UNIFORM's
    std::shared_ptr<MaterialUniform>        materialUniform = nullptr;
    std::shared_ptr<AnimationUniform>       animationUniform = nullptr;

private:
    void StartResources(std::shared_ptr<ShaderModule> shader_ptr);
    std::vector<VkWriteDescriptorSet> GetDescriptorWrites(std::shared_ptr<ShaderModule> shader_ptr, uint32_t frameIdx, const MaterialData& materialData);
    VkDescriptorBufferInfo GetBufferInfo(VkBuffer buffer, VkDeviceSize bufferSize);
    void SetDescriptorWrite(VkWriteDescriptorSet& descriptorWrite, uint32_t binding, VkBuffer buffer, VkDeviceSize bufferSize, uint32_t frameIdx);

public:
    DescriptorBuffer();
    DescriptorBuffer(std::shared_ptr<ShaderModule> shader_ptr);
    void CreateDescriptorSets(std::shared_ptr<ShaderModule> shader_ptr, const MaterialData& materialData);
};

#endif // !DESCRIPTOR_BUFFER_H

