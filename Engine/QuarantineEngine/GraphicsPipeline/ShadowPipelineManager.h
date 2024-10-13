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

typedef std::unordered_map<std::string, std::pair<std::shared_ptr<ShadowPipelineModule>, std::shared_ptr<VkRenderPass>>> ShadowResourcesMap;

class ShadowPipelineManager
{
private:
    ShadowResourcesMap _shadowPipelines;

public:
    static ShadowPipelineManager* instance;

private:
    std::string CheckName(std::string pipelineName);

public:
    ShadowPipelineManager();
    static ShadowPipelineManager* getInstance();
    static void ResetInstance();
    std::shared_ptr<ShadowPipelineModule> GetPipeline(std::string pipelineName);
    std::shared_ptr<ShadowPipelineModule> RegisterNewShadowPipeline(ShaderModule& shader, VkDescriptorSetLayout descriptorLayout, GraphicsPipelineData pipelineData);
    bool Exists(std::string pipelineName);
    void CleanShadowPipelines();
    void RecreateShadowPipeline(ShaderModule shader, VkDescriptorSetLayout descriptorLayout);
    void CleanLastResources();
};

#endif // !SHADOW_PIPELINE_MANAGER_H
