#pragma once
#ifndef GRAPHICS_PIPELINE_BASE_H
#define GRAPHICS_PIPELINE_BASE_H

#include "ShaderModule.h"
#include "DescriptorModule.h"
#include "DepthBufferModule.h"
#include "AntiAliasingModule.h"
class DepthBufferModule;

class GraphicsPipelineBase
{
public:
    VkRenderPass                renderPass;
    VkPipeline                  graphicsPipeline;
    VkPipelineLayout            pipelineLayout;
protected:
    DeviceModule* deviceModule;
    AntiAliasingModule* antialias_ptr;

public:
    GraphicsPipelineBase();
    virtual ~GraphicsPipelineBase() = 0;
    void createRenderPass(VkFormat& swapChainImageFormat, DepthBufferModule& depthBufferModule);
    virtual void createGraphicsPipeline(VkExtent2D& swapChainExtent, VkDescriptorSetLayout& descriptorSetLayout) = 0;
    virtual void addShaderModules(std::shared_ptr<ShaderModule> shader_module) = 0;
    void addAntialiasingModule(AntiAliasingModule& antialiasingModule);
    virtual void cleanup();
};

#endif
