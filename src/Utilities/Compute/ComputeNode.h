#pragma once

#ifndef COMPUTE_NODE_H
#define COMPUTE_NODE_H

#include <ComputePipelineModule.h>
#include <ComputeDescriptorBuffer.h>
#include "QEGameComponent.h"
#include "ShaderModule.h"
#include <string>

#define CEIL_DIV(x, y) (((x) + (y) - 1) / (y))

class ComputeNode : public QEGameComponent
{
    REFLECTABLE_COMPONENT(ComputeNode)
private:
    DeviceModule*                               deviceModule = nullptr;
    std::shared_ptr<ShaderModule>               computeShader = nullptr;

    REFLECT_PROPERTY(std::string, computeShaderPath)
    REFLECT_PROPERTY(uint32_t, widthImage)
    REFLECT_PROPERTY(uint32_t, heightImage)
public:
    std::shared_ptr<ComputeDescriptorBuffer>    computeDescriptor = nullptr;

    REFLECT_PROPERTY(uint32_t, NElements)
    REFLECT_PROPERTY(bool, UseDependencyBuffer)
    REFLECT_PROPERTY(bool, OnDemandCompute)
    REFLECT_PROPERTY(bool, Compute)

public:
    ComputeNode();
    ComputeNode(std::string computeShaderPath);
    ComputeNode(std::shared_ptr<ShaderModule> shader_ptr);
    void cleanup();
    void InitializeComputeBuffer(uint32_t idBuffer, uint32_t bufferSize);
    void FillComputeBuffer(uint32_t numElements, unsigned long long elementType, void* data);
    void FillComputeBuffer(uint32_t ssboIndex, VkBuffer buffer, uint32_t bufferSize);
    void InitializeComputeNode();
    void InitializeOutputTextureComputeNode(uint32_t width, uint32_t height, VkFormat format);
    void DispatchCommandBuffer(VkCommandBuffer commandBuffer, uint32_t currentFrame);

private:
    void UpdateOutputTextureState();

    void QEStart() override;
    void QEInit() override;
    void QEUpdate() override;
    void QEDestroy() override;
};

#endif // !COMPUTE_NODE_H


