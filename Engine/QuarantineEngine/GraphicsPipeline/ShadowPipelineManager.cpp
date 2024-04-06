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
    std::unordered_map<std::string, std::shared_ptr<ShadowPipelineModule>>::const_iterator got;

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

    std::unordered_map<std::string, std::shared_ptr<ShadowPipelineModule>>::const_iterator got = _shadowPipelines.find(namePipeline);

    if (got == _shadowPipelines.end())
        return nullptr;

    return _shadowPipelines[namePipeline];
}

void ShadowPipelineManager::AddShadowPipeline(const char* namePipeline, std::shared_ptr<ShadowPipelineModule> gp_ptr)
{
    if (gp_ptr->renderPass == nullptr)
        gp_ptr->renderPass = this->defaultRenderPass;
    _shadowPipelines[namePipeline] = gp_ptr;
}

void ShadowPipelineManager::AddShadowPipeline(std::string& namePipeline, std::shared_ptr<ShadowPipelineModule> gp_ptr)
{
    if (gp_ptr->renderPass == nullptr)
        gp_ptr->renderPass = this->defaultRenderPass;
    _shadowPipelines[namePipeline] = gp_ptr;
}

void ShadowPipelineManager::AddShadowPipeline(std::string& namePipeline, ShadowPipelineModule gp)
{
    std::shared_ptr<ShadowPipelineModule> gp_ptr = std::make_shared<ShadowPipelineModule>(gp);
    if (gp_ptr->renderPass == nullptr)
        gp_ptr->renderPass = this->defaultRenderPass;
    _shadowPipelines[namePipeline] = gp_ptr;
}

bool ShadowPipelineManager::Exists(std::string pipelineName)
{
    std::unordered_map<std::string, std::shared_ptr<ShadowPipelineModule>>::const_iterator got = _shadowPipelines.find(pipelineName);

    if (got == _shadowPipelines.end())
        return false;

    return true;
}

void ShadowPipelineManager::RegisterDefaultRenderPass(VkRenderPass renderPass)
{
    if (this->defaultRenderPass != nullptr)
        this->defaultRenderPass.reset();
    this->defaultRenderPass = std::make_shared<VkRenderPass>(renderPass);
}

void ShadowPipelineManager::CleanShadowPipeline()
{
    for (auto gPipeline : this->_shadowPipelines)
    {
        gPipeline.second->CleanPipelineData();
    }
}

void ShadowPipelineManager::RecreateShadowPipeline(ShaderModule shader, VkDescriptorSetLayout descriptorLayout)
{
    this->_shadowPipelines[shader.id]->renderPass = this->defaultRenderPass;
    this->_shadowPipelines[shader.id]->CompileShadowPipeline(shader.shaderStages, shader.vertexInputInfo, descriptorLayout);
}

std::shared_ptr<ShadowPipelineModule> ShadowPipelineManager::RegisterNewShadowPipeline(ShaderModule& shader, VkDescriptorSetLayout descriptorLayout, GraphicsPipelineData pipelineData)
{
    this->_shadowPipelines[shader.id] = std::make_shared<ShadowPipelineModule>();
    this->_shadowPipelines[shader.id]->PoligonMode = pipelineData.polygonMode;
    this->_shadowPipelines[shader.id]->inputTopology = pipelineData.topology;
    this->_shadowPipelines[shader.id]->lineWidth = pipelineData.lineWidth;
    this->_shadowPipelines[shader.id]->renderPass = this->defaultRenderPass;
    this->_shadowPipelines[shader.id]->CompileShadowPipeline(shader.shaderStages, shader.vertexInputInfo, descriptorLayout);
    return this->_shadowPipelines[shader.id];
}
