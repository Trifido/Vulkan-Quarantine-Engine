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
    VkDescriptorImageInfo inputImageInfo_2;
    VkDescriptorImageInfo outputImageInfo;
    std::shared_ptr<DeltaTimeUniform> deltaTimeUniform;
    std::shared_ptr<SunUniform> sunUniform;

    uint32_t _numSSBOs = 0;

public:
    std::vector<VkDescriptorSet>    descriptorSets;
    std::vector<std::shared_ptr<CustomTexture>>  inputTextures = {};
    std::shared_ptr<CustomTexture>  outputTexture = nullptr;

    std::vector<std::shared_ptr<UniformBufferObject>>    ssboData;
    std::vector<VkDeviceSize>                            ssboSize;

    std::unordered_map<std::string, std::shared_ptr<UniformBufferObject>>  ubos;
    std::unordered_map<std::string, VkDeviceSize> uboSizes;
    bool IsProgressiveComputation = false;

private:
    void StartResources(std::shared_ptr<ShaderModule> shader_ptr);
    VkDescriptorBufferInfo GetBufferInfo(VkBuffer buffer, VkDeviceSize bufferSize);
    std::vector<VkWriteDescriptorSet> GetDescriptorWrites(std::shared_ptr<ShaderModule> shader_ptr, uint32_t frameIdx);
    void SetDescriptorWrite(VkWriteDescriptorSet& descriptorWrite, VkDescriptorType descriptorType, uint32_t binding, VkBuffer buffer, VkDeviceSize bufferSize, uint32_t frameIdx);

public:
    ComputeDescriptorBuffer();
    ComputeDescriptorBuffer(std::shared_ptr<ShaderModule> shader_ptr);
    void InitializeDescriptorSets(std::shared_ptr<ShaderModule> shader_ptr);
    void InitializeSSBOData();
    void AssignSSBO(std::shared_ptr<UniformBufferObject> ssbo, VkDeviceSize size);
    void UpdateUBODeltaTime();
    void Cleanup();
};

#endif // !COMPUTE_DESCRIPTOR_BUFFER_H
