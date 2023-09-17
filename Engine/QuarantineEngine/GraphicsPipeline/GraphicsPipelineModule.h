#pragma once
#ifndef GRAPHICS_PIPELINE_MODULE_H
#define GRAPHICS_PIPELINE_MODULE_H

#include "ShaderModule.h"
#include <AntiAliasingModule.h>
#include <PipelineModule.h>

class GraphicsPipelineModule : public PipelineModule
{
private:
    SwapChainModule*    swapChainModule = nullptr;
    AntiAliasingModule* antialiasingModule = nullptr;
    VkBool32            depthBufferMode;

public:
    VkPrimitiveTopology inputTopology = VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    enum PolygonRenderType
    {
        FILL,
        LINE,
        POINT
    };

    enum DepthBufferMode
    {
        ENABLED,
        DISABLED
    };

    VkPolygonMode PoligonMode = VkPolygonMode::VK_POLYGON_MODE_FILL;
    std::shared_ptr<VkRenderPass> renderPass = nullptr;

public:
    GraphicsPipelineModule();
    ~GraphicsPipelineModule();
    void CompileGraphicsPipeline(std::vector<VkPipelineShaderStageCreateInfo> shaderInfo, VkPipelineVertexInputStateCreateInfo vertexInfo, VkDescriptorSetLayout descriptorLayout);
    void cleanup(VkPipeline pipeline, VkPipelineLayout pipelineLayout);

private:
    void updatePolygonMode(PolygonRenderType polygonType);
    void updateDepthBufferMode(DepthBufferMode depthBufferMode);
};

#endif
