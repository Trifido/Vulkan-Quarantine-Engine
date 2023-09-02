#pragma once

#ifndef COMPUTE_PIPELINE_MANAGER_H
#define COMPUTE_PIPELINE_MANAGER_H

#include <unordered_map>
#include <memory>
#include <string>

#include <vulkan/vulkan.h>
#include <ComputePipelineModule.h>

class ComputePipelineModule;
class ShaderModule;

class ComputePipelineManager
{
private:
    std::unordered_map<std::string, std::shared_ptr<ComputePipelineModule>> _computePipelines;

public:
    static ComputePipelineManager* instance;

private:
    std::string CheckName(std::string pipelineName);

public:
    static ComputePipelineManager* getInstance();
    static void ResetInstance();
    std::shared_ptr<ComputePipelineModule> GetPipeline(std::string pipelineName);
    void AddComputePipeline(const char* pipelineName, std::shared_ptr<ComputePipelineModule> gp_ptr);
    void AddComputePipeline(std::string& pipelineName, std::shared_ptr<ComputePipelineModule> gp_ptr);
    void AddComputePipeline(std::string& pipelineName, ComputePipelineModule gp);
    std::shared_ptr<ComputePipelineModule> RegisterNewComputePipeline(ShaderModule shader, VkDescriptorSetLayout descriptorLayout);
    bool Exists(std::string pipelineName);
    void CleanComputePipeline();
    void RecreateComputePipeline(ShaderModule shader, VkDescriptorSetLayout descriptorLayout);
};

#endif // !COMPUTE_PIPELINE_MANAGER_H


