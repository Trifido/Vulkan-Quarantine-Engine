#include "ComputePipelineManager.h"
#include <iostream>

std::string ComputePipelineManager::CheckName(std::string pipelineName)
{
    std::unordered_map<std::string, std::shared_ptr<ComputePipelineModule>>::const_iterator got;

    std::string newName = pipelineName;
    unsigned int id = 0;

    do
    {
        got = _computePipelines.find(newName);

        if (got != _computePipelines.end())
        {
            id++;
            newName = pipelineName + "_" + std::to_string(id);
        }
    } while (got != _computePipelines.end());

    return newName;
}

std::shared_ptr<ComputePipelineModule> ComputePipelineManager::GetPipeline(std::string pipelineName)
{
    if (_computePipelines.empty())
        return nullptr;

    std::unordered_map<std::string, std::shared_ptr<ComputePipelineModule>>::const_iterator got = _computePipelines.find(pipelineName);

    if (got == _computePipelines.end())
        return nullptr;

    return _computePipelines[pipelineName];
}

void ComputePipelineManager::AddComputePipeline(const char* pipelineName, std::shared_ptr<ComputePipelineModule> gp_ptr)
{
    _computePipelines[pipelineName] = gp_ptr;
}

void ComputePipelineManager::AddComputePipeline(std::string& pipelineName, std::shared_ptr<ComputePipelineModule> gp_ptr)
{
    _computePipelines[pipelineName] = gp_ptr;
}

void ComputePipelineManager::AddComputePipeline(std::string& pipelineName, ComputePipelineModule gp)
{
    std::shared_ptr<ComputePipelineModule> gp_ptr = std::make_shared<ComputePipelineModule>(gp);
    _computePipelines[pipelineName] = gp_ptr;
}

std::shared_ptr<ComputePipelineModule> ComputePipelineManager::RegisterNewComputePipeline(ShaderModule shader, std::vector<VkDescriptorSetLayout> descriptorLayouts)
{
    this->_computePipelines[shader.id] = std::make_shared<ComputePipelineModule>();
    this->_computePipelines[shader.id]->CompileComputePipeline(shader.shaderStages, descriptorLayouts);
    return this->_computePipelines[shader.id];
}

bool ComputePipelineManager::Exists(std::string pipelineName)
{
    std::unordered_map<std::string, std::shared_ptr<ComputePipelineModule>>::const_iterator got = _computePipelines.find(pipelineName);

    if (got == _computePipelines.end())
        return false;

    return true;
}

void ComputePipelineManager::CleanComputePipeline()
{
    for (auto gPipeline : this->_computePipelines)
    {
        gPipeline.second->CleanPipelineData();
    }
}

void ComputePipelineManager::RecreateComputePipeline(ShaderModule shader, std::vector<VkDescriptorSetLayout> descriptorLayouts)
{
    this->_computePipelines[shader.id]->CompileComputePipeline(shader.shaderStages, descriptorLayouts);
}
