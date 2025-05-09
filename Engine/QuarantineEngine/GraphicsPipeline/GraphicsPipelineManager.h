#pragma once

#ifndef GRAPHICS_PIPELINE_MANAGER_H
#define GRAPHICS_PIPELINE_MANAGER_H

#include <unordered_map>
#include <memory>
#include <string>
#include <vulkan/vulkan.h>
#include <GraphicsPipelineModule.h>
#include <GraphicsPipelineData.h>
#include <QESingleton.h>

class GraphicsPipelineModule;
class ShaderModule;

class GraphicsPipelineManager : public QESingleton<GraphicsPipelineManager>

{
private:
    friend class QESingleton<GraphicsPipelineManager>; // Permitir acceso al constructor
    std::unordered_map<std::string, std::shared_ptr<GraphicsPipelineModule>> _graphicsPipelines;
    std::shared_ptr<VkRenderPass> defaultRenderPass = nullptr;

private:
    std::string CheckName(std::string pipelineName);

public:
    std::shared_ptr<GraphicsPipelineModule> GetPipeline(std::string pipelineName);
    void AddGraphicsPipeline(const char* pipelineName, std::shared_ptr<GraphicsPipelineModule> gp_ptr);
    void AddGraphicsPipeline(std::string& pipelineName, std::shared_ptr<GraphicsPipelineModule> gp_ptr);
    void AddGraphicsPipeline(std::string& pipelineName, GraphicsPipelineModule gp);
    std::shared_ptr<GraphicsPipelineModule> RegisterNewGraphicsPipeline(ShaderModule shader, std::vector<VkDescriptorSetLayout> descriptorLayouts, GraphicsPipelineData pipelineData);
    bool Exists(std::string pipelineName);
    void RegisterDefaultRenderPass(std::shared_ptr<VkRenderPass> renderPass);
    void CleanGraphicsPipeline();
    void RecreateGraphicsPipeline(ShaderModule shader, std::vector<VkDescriptorSetLayout> descriptorLayouts);
};

#endif
