#pragma once

#ifndef COMPUTE_NODE_H
#define COMPUTE_NODE_H

#include <ComputePipelineModule.h>
#include <ComputeDescriptorBuffer.h>
#include "GameComponent.h"
#include "ShaderModule.h"
#include <string>

class ComputeNode : public GameComponent
{
private:
    DeviceModule*                               deviceModule = nullptr;
    std::shared_ptr<ShaderModule>               computeShader = nullptr;

public:
    std::shared_ptr<ComputeDescriptorBuffer>    computeDescriptor = nullptr;
    uint32_t NElements = 0;
    bool UseDependencyBuffer = false;

public:
    ComputeNode();
    ComputeNode(std::string computeShaderPath);
    ComputeNode(std::shared_ptr<ShaderModule> shader_ptr);
    void cleanup();
    void InitializeComputeBuffer(uint32_t idBuffer, uint32_t bufferSize);
    void FillComputeBuffer(size_t numElements, unsigned long long elementType, void* data);
    void FillComputeBuffer(VkBuffer buffer, uint32_t bufferSize);
    void InitializeComputeNode();
    void DispatchCommandBuffer(VkCommandBuffer commandBuffer, uint32_t currentFrame);
    void UpdateComputeDescriptor();
};

#endif // !COMPUTE_NODE_H


