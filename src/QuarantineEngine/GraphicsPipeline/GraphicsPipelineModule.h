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
    VkCullModeFlags cullMode = VK_CULL_MODE_BACK_BIT;
    VkFrontFace frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

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
    float lineWidth = 1.0f;
    VkBool32 depthTestEnabled = VK_TRUE;
    VkBool32 depthWriteEnabled = VK_TRUE;

public:
    GraphicsPipelineModule();
    ~GraphicsPipelineModule();
    void CompileGraphicsPipeline(std::vector<VkPipelineShaderStageCreateInfo> shaderInfo, VkPipelineVertexInputStateCreateInfo vertexInfo, std::vector<VkDescriptorSetLayout> descriptorLayouts);
    void cleanup(VkPipeline pipeline, VkPipelineLayout pipelineLayout);

private:
    void updatePolygonMode(PolygonRenderType polygonType);
    void updateDepthBufferMode(DepthBufferMode depthBufferMode);
};



namespace QE
{
    using ::GraphicsPipelineModule;
} // namespace QE
// QE namespace aliases
#endif
