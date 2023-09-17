#pragma once

#ifndef GRAPHICS_PIPELINE_MANAGER_H
#define GRAPHICS_PIPELINE_MANAGER_H

#include <unordered_map>
#include <memory>
#include <string>

#include <vulkan/vulkan.h>
#include <GraphicsPipelineModule.h>
#include <GraphicsPipelineData.h>

class GraphicsPipelineModule;
class ShaderModule;

class GraphicsPipelineManager
{
private:
    std::unordered_map<std::string, std::shared_ptr<GraphicsPipelineModule>> _graphicsPipelines;
    std::shared_ptr<VkRenderPass> defaultRenderPass = nullptr;

public:
    static GraphicsPipelineManager* instance;

private:
    std::string CheckName(std::string pipelineName);

public:
    static GraphicsPipelineManager* getInstance();
    static void ResetInstance();
    std::shared_ptr<GraphicsPipelineModule> GetPipeline(std::string pipelineName);
    void AddGraphicsPipeline(const char* pipelineName, std::shared_ptr<GraphicsPipelineModule> gp_ptr);
    void AddGraphicsPipeline(std::string& pipelineName, std::shared_ptr<GraphicsPipelineModule> gp_ptr);
    void AddGraphicsPipeline(std::string& pipelineName, GraphicsPipelineModule gp);
    std::shared_ptr<GraphicsPipelineModule> RegisterNewGraphicsPipeline(ShaderModule shader, VkDescriptorSetLayout descriptorLayout, GraphicsPipelineData pipelineData);
    bool Exists(std::string pipelineName);
    void RegisterDefaultRenderPass(VkRenderPass renderPass);
    void CleanGraphicsPipeline();
    void RecreateGraphicsPipeline(ShaderModule shader, VkDescriptorSetLayout descriptorLayout);
};

#endif
