#include "ShadowPipelineManager.h"
#include "ShaderModule.h"
#include <iostream>

ShadowPipelineManager* ShadowPipelineManager::instance = nullptr;

ShadowPipelineManager::ShadowPipelineManager()
{
    this->_shadowPipelines.reserve(8);
}

std::string ShadowPipelineManager::CheckName(std::string namePipeline)
{
    ShadowResourcesMap::const_iterator got;

    std::string newName = namePipeline;
    unsigned int id = 0;

    do
    {
        got = _shadowPipelines.find(newName);

        if (got != _shadowPipelines.end())
        {
            id++;
            newName = namePipeline + "_" + std::to_string(id);
        }
    } while (got != _shadowPipelines.end());

    return newName;
}

ShadowPipelineManager* ShadowPipelineManager::getInstance()
{
    if (instance == NULL)
        instance = new ShadowPipelineManager();

    return instance;
}

void ShadowPipelineManager::ResetInstance()
{
    delete instance;
    instance = nullptr;
}

std::shared_ptr<ShadowPipelineModule> ShadowPipelineManager::GetPipeline(std::string namePipeline)
{
    if (_shadowPipelines.empty())
        return nullptr;

    ShadowResourcesMap::const_iterator got = _shadowPipelines.find(namePipeline);

    if (got == _shadowPipelines.end())
        return nullptr;

    return _shadowPipelines[namePipeline].first;
}

bool ShadowPipelineManager::Exists(std::string pipelineName)
{
    ShadowResourcesMap::const_iterator got = _shadowPipelines.find(pipelineName);

    if (got == _shadowPipelines.end())
        return false;

    return true;
}

void ShadowPipelineManager::CleanShadowPipelines()
{
    for (auto gPipeline : this->_shadowPipelines)
    {
        gPipeline.second.first->CleanPipelineData();
    }
}

void ShadowPipelineManager::RecreateShadowPipeline(ShaderModule shader, VkDescriptorSetLayout descriptorLayout)
{
    this->_shadowPipelines[shader.id].first->CompileShadowPipeline(shader.shaderStages, shader.vertexInputInfo, descriptorLayout);
}

void ShadowPipelineManager::CleanLastResources()
{
    this->_shadowPipelines.clear();
}

std::shared_ptr<ShadowPipelineModule> ShadowPipelineManager::RegisterNewShadowPipeline(ShaderModule& shader, VkDescriptorSetLayout descriptorLayout, GraphicsPipelineData pipelineData)
{
    this->_shadowPipelines[shader.id].first = std::make_shared<ShadowPipelineModule>();
    this->_shadowPipelines[shader.id].first->PoligonMode = pipelineData.polygonMode;
    this->_shadowPipelines[shader.id].first->inputTopology = pipelineData.topology;
    this->_shadowPipelines[shader.id].first->lineWidth = pipelineData.lineWidth;

    this->_shadowPipelines[shader.id].second = pipelineData.renderPass;
    this->_shadowPipelines[shader.id].first->renderPass = this->_shadowPipelines[shader.id].second;

    this->_shadowPipelines[shader.id].first->SetShadowMappingMode(pipelineData.shadowMode);
    this->_shadowPipelines[shader.id].first->CompileShadowPipeline(shader.shaderStages, shader.vertexInputInfo, descriptorLayout);
    return this->_shadowPipelines[shader.id].first;
}
