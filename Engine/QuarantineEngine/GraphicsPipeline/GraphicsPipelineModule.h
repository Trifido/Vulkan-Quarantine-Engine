#pragma once
#ifndef GRAPHICS_PIPELINE_MODULE_H
#define GRAPHICS_PIPELINE_MODULE_H

#include "ShaderModule.h"
#include "DescriptorModule.h"
#include <AntiAliasingModule.h>

class GraphicsPipelineModule
{
private:
    DeviceModule*       deviceModule = nullptr;
    SwapChainModule*    swapChainModule = nullptr;
    AntiAliasingModule* antialiasingModule = nullptr;
    VkPolygonMode       PoligonMode;
    VkBool32            depthBufferMode;

public:
    enum InputTopology
    {
        TRIANGLE_LIST,
        LINES,
        POINTS
    } inputTopology = TRIANGLE_LIST;

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
public:
    GraphicsPipelineModule();
    ~GraphicsPipelineModule();
    void CreateGraphicsPipeline(VkPipeline& pipeline, VkPipelineLayout& pipelineLayout, std::shared_ptr<ShaderModule> shader, std::shared_ptr<DescriptorModule> descriptor_ptr, VkRenderPass renderPass);
    void cleanup(VkPipeline pipeline, VkPipelineLayout pipelineLayout);

private:
    void updatePolygonMode(PolygonRenderType polygonType);
    void updateDepthBufferMode(DepthBufferMode depthBufferMode);
    VkPrimitiveTopology getInputTopology();
};

#endif
