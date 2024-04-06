#pragma once

#ifndef SHADOW_PIPELINE_MANAGER_H
#define SHADOW_PIPELINE_MANAGER_H

#include <unordered_map>
#include <memory>
#include <string>

#include <vulkan/vulkan.h>
#include <ShadowPipelineModule.h>
#include <GraphicsPipelineData.h>

class ShaderModule;

class ShadowPipelineManager
{
private:
    std::unordered_map<std::string, std::shared_ptr<ShadowPipelineModule>> _shadowPipelines;
    std::shared_ptr<VkRenderPass> defaultRenderPass = nullptr;

public:
    static ShadowPipelineManager* instance;

private:
    std::string CheckName(std::string pipelineName);

public:
    ShadowPipelineManager();
    static ShadowPipelineManager* getInstance();
    static void ResetInstance();
    std::shared_ptr<ShadowPipelineModule> GetPipeline(std::string pipelineName);
    void AddShadowPipeline(const char* pipelineName, std::shared_ptr<ShadowPipelineModule> gp_ptr);
    void AddShadowPipeline(std::string& pipelineName, std::shared_ptr<ShadowPipelineModule> gp_ptr);
    void AddShadowPipeline(std::string& pipelineName, ShadowPipelineModule gp);
    std::shared_ptr<ShadowPipelineModule> RegisterNewShadowPipeline(ShaderModule& shader, VkDescriptorSetLayout descriptorLayout, GraphicsPipelineData pipelineData);
    bool Exists(std::string pipelineName);
    void RegisterDefaultRenderPass(VkRenderPass renderPass);
    void CleanShadowPipeline();
    void RecreateShadowPipeline(ShaderModule shader, VkDescriptorSetLayout descriptorLayout);
};

#endif // !SHADOW_PIPELINE_MANAGER_H
