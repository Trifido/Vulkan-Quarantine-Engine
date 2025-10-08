#pragma once

#ifndef DESCRIPTOR_BUFFER_H
#define DESCRIPTOR_BUFFER_H

#include "CustomTexture.h"
#include "ShaderModule.h"
#include "QECamera.h"
#include "LightManager.h"
#include "UBO.h"
#include <MaterialData.h>
#include <Meshlet.h>

class LightManager;

class DescriptorBuffer
{
private:
    DeviceModule*   deviceModule = nullptr;
    LightManager*   lightManager = nullptr;
    SwapChainModule* swapChainModule = nullptr;
    std::shared_ptr<QECamera>   camera = nullptr;
    std::shared_ptr<Meshlet> meshlets_ptr = nullptr;

    VkDescriptorPool                descriptorPool;

    uint32_t    numUBOs = 0;
    uint32_t    numSSBOs = 0;
    uint32_t    numBinding = 0;
    bool        hasAnimationProperties = false;

    std::vector<VkDescriptorBufferInfo> buffersInfo;
    std::vector<VkDescriptorImageInfo> imageInfo;

public:
    std::vector<VkDescriptorSet>    descriptorSets;

    std::unordered_map<std::string, std::shared_ptr<UniformBufferObject>>    ssboData;
    std::unordered_map<std::string, VkDeviceSize>                            ssboSize;

    //UBO's
    std::unordered_map<std::string, std::shared_ptr<UniformBufferObject>>  ubos;
    std::unordered_map<std::string, VkDeviceSize> uboSizes;
    std::shared_ptr<std::vector<std::shared_ptr<CustomTexture>>> textures;

private:
    void StartResources(std::shared_ptr<ShaderModule> shader_ptr);
    std::vector<VkWriteDescriptorSet> GetDescriptorWrites(std::shared_ptr<ShaderModule> shader_ptr, uint32_t frameIdx);
    VkDescriptorBufferInfo GetBufferInfo(VkBuffer buffer, VkDeviceSize bufferSize);
    void SetDescriptorWrite(VkWriteDescriptorSet& descriptorWrite, VkDescriptorType descriptorType, uint32_t binding, VkBuffer buffer, VkDeviceSize bufferSize, uint32_t frameIdx);
    void CleanDescriptorSetPool();

public:
    DescriptorBuffer();
    DescriptorBuffer(std::shared_ptr<ShaderModule> shader_ptr);
    void SetMeshletBuffers(std::shared_ptr<Meshlet> meshlets_ptr);
    void InitializeDescriptorSets(std::shared_ptr<ShaderModule> shader_ptr);
    //void InitializeShadowMapDescritorSets(std::shared_ptr<ShaderModule> shader_ptr, std::shared_ptr<UniformBufferObject> lightUniformBuffer, VkDeviceSize sizeBuffer);
    VkDescriptorSet* getDescriptorSet(size_t id) { return &descriptorSets.at(id); }
    void CleanLastResources();
    void Cleanup();
};

#endif // !DESCRIPTOR_BUFFER_H

