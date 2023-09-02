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
    uint32_t nElements = 0;

public:
    std::shared_ptr<ComputeDescriptorBuffer>    computeDescriptor = nullptr;

public:
    ComputeNode();
    ComputeNode(std::string computeShaderPath);
    ComputeNode(std::shared_ptr<ShaderModule> shader_ptr);
    void cleanup();
    void FillComputeBuffer(size_t numElements, unsigned long long elementType, void* data);
    void InitializeComputeNode();
    void DispatchCommandBuffer(VkCommandBuffer commandBuffer, uint32_t currentFrame);
};

#endif // !COMPUTE_NODE_H


