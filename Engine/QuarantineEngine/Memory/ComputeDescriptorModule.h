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
    VkDescriptorSetLayout computeDescriptorSetLayout;

    std::vector<VkBuffer> uniformBuffers;
    std::vector<VkDeviceMemory> uniformBuffersMemory;
    std::vector<void*> uniformBuffersMapped;

public:
    void createComputeDescriptorSetLayout();
    void createDescriptorPool();
    void createComputeDescriptorSets();
    void createUniformBuffers();
    void cleanup();
};

#endif // !COMPUTE_DESCRIPTOR_MODULE_H

