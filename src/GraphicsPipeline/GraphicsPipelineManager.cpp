#include "GraphicsPipelineManager.h"
#include <iostream>

std::string GraphicsPipelineManager::CheckName(std::string namePipeline)
{
	std::unordered_map<std::string, std::shared_ptr<GraphicsPipelineModule>>::const_iterator got;

	std::string newName = namePipeline;
	unsigned int id = 0;

	do
	{
		got = _graphicsPipelines.find(newName);

		if (got != _graphicsPipelines.end())
		{
			id++;
			newName = namePipeline + "_" + std::to_string(id);
		}
	} while (got != _graphicsPipelines.end());

	return newName;
}

std::shared_ptr<GraphicsPipelineModule> GraphicsPipelineManager::GetPipeline(std::string namePipeline)
{
    if (_graphicsPipelines.empty())
        return nullptr;

    std::unordered_map<std::string, std::shared_ptr<GraphicsPipelineModule>>::const_iterator got = _graphicsPipelines.find(namePipeline);

    if (got == _graphicsPipelines.end())
        return nullptr;

    return _graphicsPipelines[namePipeline];
}

void GraphicsPipelineManager::AddGraphicsPipeline(const char* namePipeline, std::shared_ptr<GraphicsPipelineModule> gp_ptr)
{
    if(gp_ptr->renderPass == nullptr)
        gp_ptr->renderPass = this->defaultRenderPass;
    _graphicsPipelines[namePipeline] = gp_ptr;
}

void GraphicsPipelineManager::AddGraphicsPipeline(std::string& namePipeline, std::shared_ptr<GraphicsPipelineModule> gp_ptr)
{
    if (gp_ptr->renderPass == nullptr)
        gp_ptr->renderPass = this->defaultRenderPass;
    _graphicsPipelines[namePipeline] = gp_ptr;
}

void GraphicsPipelineManager::AddGraphicsPipeline(std::string& namePipeline, GraphicsPipelineModule gp)
{
    std::shared_ptr<GraphicsPipelineModule> gp_ptr = std::make_shared<GraphicsPipelineModule>(gp);
    if (gp_ptr->renderPass == nullptr)
        gp_ptr->renderPass = this->defaultRenderPass;
    _graphicsPipelines[namePipeline] = gp_ptr;
}

bool GraphicsPipelineManager::Exists(std::string pipelineName)
{
    std::unordered_map<std::string, std::shared_ptr<GraphicsPipelineModule>>::const_iterator got = _graphicsPipelines.find(pipelineName);

    if (got == _graphicsPipelines.end())
        return false;

    return true;
}

void GraphicsPipelineManager::RegisterDefaultRenderPass(std::shared_ptr<VkRenderPass> renderPass)
{
    if (this->defaultRenderPass != nullptr)
        this->defaultRenderPass.reset();
    this->defaultRenderPass = renderPass;
}

void GraphicsPipelineManager::CleanGraphicsPipeline()
{
    for (auto gPipeline : this->_graphicsPipelines)
    {
        gPipeline.second->CleanPipelineData();
    }
}

void GraphicsPipelineManager::RecreateGraphicsPipeline(ShaderModule shader, std::vector<VkDescriptorSetLayout> descriptorLayouts)
{
    this->_graphicsPipelines[shader.id]->renderPass = this->defaultRenderPass;
    this->_graphicsPipelines[shader.id]->CompileGraphicsPipeline(shader.shaderStages, shader.vertexInputInfo, descriptorLayouts);
}

std::shared_ptr<GraphicsPipelineModule> GraphicsPipelineManager::RegisterNewGraphicsPipeline(ShaderModule shader, std::vector<VkDescriptorSetLayout> descriptorLayouts, GraphicsPipelineData pipelineData)
{
    this->_graphicsPipelines[shader.id] = std::make_shared<GraphicsPipelineModule>();
    this->_graphicsPipelines[shader.id]->PoligonMode = pipelineData.polygonMode;
    this->_graphicsPipelines[shader.id]->inputTopology = pipelineData.topology;
    this->_graphicsPipelines[shader.id]->lineWidth = pipelineData.lineWidth;
    this->_graphicsPipelines[shader.id]->renderPass = this->defaultRenderPass;
    this->_graphicsPipelines[shader.id]->CompileGraphicsPipeline(shader.shaderStages, shader.vertexInputInfo, descriptorLayouts);
    return this->_graphicsPipelines[shader.id];
}
