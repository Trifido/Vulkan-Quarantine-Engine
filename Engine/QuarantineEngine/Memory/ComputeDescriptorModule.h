#pragma once

#ifndef COMPUTE_DESCRIPTOR_MODULE_H
#define COMPUTE_DESCRIPTOR_MODULE_H

#include <DeviceModule.h>

struct UniformBufferComputeObject
{
    float deltaTime = 1.0f;
};

class ComputeDescriptorModule
{
private:
    static DeviceModule* deviceModule;
    static  uint32_t NumSwapchainImages;

    VkDescriptorPool descriptorPool;
    std::vector<VkDescriptorSet> computeDescriptorSets;

    //std::vector<VkBuffer> uniformBuffers;
    //std::vector<VkDeviceMemory> uniformBuffersMemory;
    //std::vector<void*> uniformBuffersMapped;
    std::shared_ptr<std::vector<VkBuffer>> shaderStorageBuffers;
    long long unsigned storageSize;

public:
    VkDescriptorSetLayout computeDescriptorSetLayout;

public:
    void initializeDescriptor(std::shared_ptr<std::vector<VkBuffer>> shaderStorageBuffers, long long unsigned storageSize);
    void createComputeDescriptorSetLayout();
    void createDescriptorPool();
    void createComputeDescriptorSets();
    //void createUniformBuffers();
    void cleanup();
};

#endif // !COMPUTE_DESCRIPTOR_MODULE_H

