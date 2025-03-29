#pragma once

#ifndef COMPUTE_PIPELINE_MANAGER_H
#define COMPUTE_PIPELINE_MANAGER_H

#include <unordered_map>
#include <memory>
#include <string>
#include <vulkan/vulkan.h>
#include <ComputePipelineModule.h>
#include <QESingleton.h>

class ComputePipelineModule;
class ShaderModule;

class ComputePipelineManager : public QESingleton<ComputePipelineManager>
{
private:
    friend class QESingleton<ComputePipelineManager>; // Permitir acceso al constructor
    std::unordered_map<std::string, std::shared_ptr<ComputePipelineModule>> _computePipelines;

private:
    std::string CheckName(std::string pipelineName);

public:
    std::shared_ptr<ComputePipelineModule> GetPipeline(std::string pipelineName);
    void AddComputePipeline(const char* pipelineName, std::shared_ptr<ComputePipelineModule> gp_ptr);
    void AddComputePipeline(std::string& pipelineName, std::shared_ptr<ComputePipelineModule> gp_ptr);
    void AddComputePipeline(std::string& pipelineName, ComputePipelineModule gp);
    std::shared_ptr<ComputePipelineModule> RegisterNewComputePipeline(ShaderModule shader, std::vector<VkDescriptorSetLayout> descriptorLayouts);
    bool Exists(std::string pipelineName);
    void CleanComputePipeline();
    void RecreateComputePipeline(ShaderModule shader, std::vector<VkDescriptorSetLayout> descriptorLayout);
};

#endif // !COMPUTE_PIPELINE_MANAGER_H


