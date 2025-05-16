#pragma once

#ifndef SHADOW_PIPELINE_MANAGER_H
#define SHADOW_PIPELINE_MANAGER_H

#include <unordered_map>
#include <memory>
#include <string>
#include <vulkan/vulkan.h>
#include <ShadowPipelineModule.h>
#include <GraphicsPipelineData.h>
#include <QESingleton.h>

class ShaderModule;

typedef std::unordered_map<std::string, std::pair<std::shared_ptr<ShadowPipelineModule>, std::shared_ptr<VkRenderPass>>> ShadowResourcesMap;

class ShadowPipelineManager : public QESingleton<ShadowPipelineManager>
{
private:
    friend class QESingleton<ShadowPipelineManager>; // Permitir acceso al constructor
    ShadowResourcesMap _shadowPipelines;

private:
    std::string CheckName(std::string pipelineName);

public:
    ShadowPipelineManager();
    std::shared_ptr<ShadowPipelineModule> GetPipeline(std::string pipelineName);
    std::shared_ptr<ShadowPipelineModule> RegisterNewShadowPipeline(ShaderModule& shader, std::vector<VkDescriptorSetLayout> descriptorLayouts, GraphicsPipelineData pipelineData);
    bool Exists(std::string pipelineName);
    void CleanShadowPipelines();
    void RecreateShadowPipeline(ShaderModule shader, std::vector<VkDescriptorSetLayout> descriptorLayouts);
    void CleanLastResources();
};

#endif // !SHADOW_PIPELINE_MANAGER_H
