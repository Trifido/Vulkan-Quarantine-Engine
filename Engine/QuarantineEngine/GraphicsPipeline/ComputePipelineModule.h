#pragma once
#ifndef COMPUTE_PIPELINE_MODULE_H
#define COMPUTE_PIPELINE_MODULE_H

#include "ShaderModule.h"
#include "ComputeDescriptorModule.h"
#include <AntiAliasingModule.h>
#include <PipelineModule.h>

class ComputePipelineModule : public PipelineModule
{
public:
    void CompileComputePipeline(std::vector<VkPipelineShaderStageCreateInfo> shaderInfo, VkDescriptorSetLayout descriptorLayout);
    void CreateComputePipeline(VkPipeline& pipeline, VkPipelineLayout& pipelineLayout, std::shared_ptr<ShaderModule> shader, std::shared_ptr<ComputeDescriptorModule> descriptor_ptr);
    void cleanup(VkPipeline pipeline, VkPipelineLayout pipelineLayout);
};

#endif
