#pragma once
#ifndef COMPUTE_PIPELINE_MODULE_H
#define COMPUTE_PIPELINE_MODULE_H

#include "ShaderModule.h"
#include "DescriptorModule.h"
#include <AntiAliasingModule.h>

class ComputePipelineModule
{
private:
    DeviceModule*       deviceModule = nullptr;

public:
    ComputePipelineModule();
    ~ComputePipelineModule();
    void CreateComputePipeline(VkPipeline& pipeline, VkPipelineLayout& pipelineLayout, std::shared_ptr<ShaderModule> shader, std::shared_ptr<DescriptorModule> descriptor_ptr, VkRenderPass renderPass);
    void cleanup(VkPipeline pipeline, VkPipelineLayout pipelineLayout);
};

#endif
