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

    uint32_t    numUBOs = 0;
    uint32_t    numBinding = 0;
    bool        hasAnimationProperties = false;

    std::vector<VkDescriptorBufferInfo> buffersInfo;

public:
    std::vector<VkDescriptorSet>    descriptorSets;

    //UBO's
    std::shared_ptr<UniformBufferObject>    materialUBO = nullptr;
    VkDeviceSize    materialUniformSize = 0;
    std::shared_ptr<UniformBufferObject>    animationUBO = nullptr;
    VkDeviceSize    animationUniformSize = 0;
    std::shared_ptr<std::vector<std::shared_ptr<CustomTexture>>> textures;

private:
    void StartResources(std::shared_ptr<ShaderModule> shader_ptr);
    std::vector<VkWriteDescriptorSet> GetDescriptorWrites(std::shared_ptr<ShaderModule> shader_ptr, uint32_t frameIdx);
    VkDescriptorBufferInfo GetBufferInfo(VkBuffer buffer, VkDeviceSize bufferSize);
    void SetDescriptorWrite(VkWriteDescriptorSet& descriptorWrite, uint32_t binding, VkBuffer buffer, VkDeviceSize bufferSize, uint32_t frameIdx);

public:
    DescriptorBuffer();
    DescriptorBuffer(std::shared_ptr<ShaderModule> shader_ptr);
    void InitializeDescriptorSets(std::shared_ptr<ShaderModule> shader_ptr);
    VkDescriptorSet* getDescriptorSet(size_t id) { return &descriptorSets.at(id); }
};

#endif // !DESCRIPTOR_BUFFER_H

