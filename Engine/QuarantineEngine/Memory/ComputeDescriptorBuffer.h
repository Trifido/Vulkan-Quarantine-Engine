#pragma once

#ifndef COMPUTE_DESCRIPTOR_BUFFER_H
#define COMPUTE_DESCRIPTOR_BUFFER_H

#include "ShaderModule.h"
#include "CustomTexture.h"
#include "UBO.h"

class ComputeDescriptorBuffer
{
private:
    DeviceModule*       deviceModule = nullptr;
    VkDescriptorPool    descriptorPool;
    uint32_t            numBinding = 0;
    std::vector<VkDescriptorBufferInfo> buffersInfo;
    VkDescriptorImageInfo inputImageInfo;
    VkDescriptorImageInfo outputImageInfo;
    std::shared_ptr<DeltaTimeUniform> deltaTimeUniform;

public:
    std::vector<VkDescriptorSet>    descriptorSets;
    std::shared_ptr<CustomTexture>  inputTexture = nullptr;
    std::shared_ptr<CustomTexture>  outputTexture = nullptr;

    std::shared_ptr<UniformBufferObject>    ssbo = nullptr;
    VkDeviceSize                            ssboSize = 0;

    std::shared_ptr<UniformBufferObject>    uboDeltaTime = nullptr;
    VkDeviceSize                            uboDeltaTimeSize = 0;
    bool    IsProgressiveComputation = false;

private:
    void StartResources(std::shared_ptr<ShaderModule> shader_ptr);
    VkDescriptorBufferInfo GetBufferInfo(VkBuffer buffer, VkDeviceSize bufferSize);
    std::vector<VkWriteDescriptorSet> GetDescriptorWrites(std::shared_ptr<ShaderModule> shader_ptr, uint32_t frameIdx);
    void SetDescriptorWrite(VkWriteDescriptorSet& descriptorWrite, VkDescriptorType descriptorType, uint32_t binding, VkBuffer buffer, VkDeviceSize bufferSize, uint32_t frameIdx);

public:
    ComputeDescriptorBuffer();
    ComputeDescriptorBuffer(std::shared_ptr<ShaderModule> shader_ptr);
    void InitializeDescriptorSets(std::shared_ptr<ShaderModule> shader_ptr);
    void UpdateUBODeltaTime();
    void Cleanup();
};

#endif // !COMPUTE_DESCRIPTOR_BUFFER_H
