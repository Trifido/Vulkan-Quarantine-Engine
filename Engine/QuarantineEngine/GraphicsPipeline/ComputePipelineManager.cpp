#include "ComputePipelineManager.h"
#include <iostream>

ComputePipelineManager* ComputePipelineManager::instance = nullptr;

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

ComputePipelineManager* ComputePipelineManager::getInstance()
{
    if (instance == NULL)
        instance = new ComputePipelineManager();
    return instance;
}

void ComputePipelineManager::ResetInstance()
{
    delete instance;
    instance = nullptr;
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

std::shared_ptr<ComputePipelineModule> ComputePipelineManager::RegisterNewComputePipeline(ShaderModule shader, VkDescriptorSetLayout descriptorLayout)
{
    this->_computePipelines[shader.id] = std::make_shared<ComputePipelineModule>();
    this->_computePipelines[shader.id]->CompileComputePipeline(shader.shaderStages, descriptorLayout);
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

void ComputePipelineManager::RecreateComputePipeline(ShaderModule shader, VkDescriptorSetLayout descriptorLayout)
{
    this->_computePipelines[shader.id]->CompileComputePipeline(shader.shaderStages, descriptorLayout);
}
