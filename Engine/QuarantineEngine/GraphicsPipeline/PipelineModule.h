#pragma once

#ifndef PIPELINE_MODULE_H
#define PIPELINE_MODULE_H

#include <Numbered.h>
#include <DeviceModule.h>

class PipelineModule : public Numbered
{
protected:
    DeviceModule* deviceModule = nullptr;

public:
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;

    PipelineModule();
    ~PipelineModule();
    virtual void CompileComputePipeline(std::vector<VkPipelineShaderStageCreateInfo> shaderInfo, VkDescriptorSetLayout descriptorLayout);
    virtual void CompileGraphicsPipeline(std::vector<VkPipelineShaderStageCreateInfo> shaderInfo, VkPipelineVertexInputStateCreateInfo vertexInfo, VkDescriptorSetLayout descriptorLayout);
    virtual void CompileShadowPipeline(std::vector<VkPipelineShaderStageCreateInfo> shaderInfo, VkPipelineVertexInputStateCreateInfo vertexInfo, VkDescriptorSetLayout descriptorLayout);
    void CleanPipelineData();
};

#endif
