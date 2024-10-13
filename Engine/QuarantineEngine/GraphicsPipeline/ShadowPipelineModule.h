#pragma once

#ifndef SHADOW_PIPELINE_MODULE_H
#define SHADOW_PIPELINE_MODULE_H

#include <PipelineModule.h>
#include <SwapChainModule.h>
#include <AntiAliasingModule.h>
#include <ShadowMappingMode.h>

class ShadowPipelineModule : public PipelineModule
{
private:
    SwapChainModule* swapChainModule = nullptr;
    AntiAliasingModule* antialiasingModule = nullptr;
    VkBool32            depthBufferMode;
    ShadowMappingMode shadowMode = ShadowMappingMode::NONE;
public:
    VkPrimitiveTopology inputTopology = VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    VkPolygonMode PoligonMode = VkPolygonMode::VK_POLYGON_MODE_FILL;
    std::shared_ptr<VkRenderPass> renderPass = nullptr;
    float lineWidth = 1.0f;

public:
    ShadowPipelineModule();
    ~ShadowPipelineModule();
    void SetShadowMappingMode(ShadowMappingMode shadowMode);
    ShadowMappingMode GetShadowMappingMode();
    void CompileShadowPipeline(std::vector<VkPipelineShaderStageCreateInfo> shaderInfo, VkPipelineVertexInputStateCreateInfo vertexInfo, VkDescriptorSetLayout descriptorLayout) override;
    void cleanup(VkPipeline pipeline, VkPipelineLayout pipelineLayout);

private:

    void CompileDirectionalShadowPipeline(std::vector<VkPipelineShaderStageCreateInfo> shaderInfo, VkPipelineVertexInputStateCreateInfo vertexInfo, VkDescriptorSetLayout descriptorLayout);
    void CompileOmniShadowPipeline(std::vector<VkPipelineShaderStageCreateInfo> shaderInfo, VkPipelineVertexInputStateCreateInfo vertexInfo, VkDescriptorSetLayout descriptorLayout);
};

#endif // !SHADOW_PIPELINE_MODULE_H
