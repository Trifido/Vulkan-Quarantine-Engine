#pragma once

#ifndef COMPUTE_NODE_H
#define COMPUTE_NODE_H

#include <ComputePipelineModule.h>
#include <ComputeDescriptorModule.h>
#include "GameComponent.h"
#include "ShaderModule.h"

class ComputeNode : public GameComponent
{
private:
    DeviceModule*                           deviceModule = nullptr;
    VkPipeline                              computePipeline;
    VkPipelineLayout                        computePipelineLayout;
    std::shared_ptr<ShaderModule>               computeShader = nullptr;
    std::shared_ptr<ComputePipelineModule>      computePipelineModule = nullptr;
    std::shared_ptr<ComputeDescriptorModule>    computeDescriptor = nullptr;

    uint32_t numElements;
    VkDeviceSize bufferSize;
    std::shared_ptr<std::vector<VkDeviceMemory>>  shaderStorageBuffersMemory;
public:
    std::shared_ptr<std::vector<VkBuffer>>        shaderStorageBuffers;

public:
    ComputeNode();
    ComputeNode(std::shared_ptr<ShaderModule> shader_ptr);
    void cleanup();
    void AddComputePipeline(std::shared_ptr<ComputePipelineModule> computePipelineModule_ptr);
    void FillComputeBuffer(size_t numElements, unsigned long long elementType, void* data);
    void InitializeComputeNode();
    void InitializeDescriptor();
    void BindCommandBuffer(VkCommandBuffer commandBuffer, uint32_t currentFrame);
};

#endif // !COMPUTE_NODE_H


