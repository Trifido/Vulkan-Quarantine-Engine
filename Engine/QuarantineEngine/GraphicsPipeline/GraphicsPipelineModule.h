#pragma once
#ifndef GRAPHICS_PIPELINE_MODULE_H
#define GRAPHICS_PIPELINE_MODULE_H

#include "ShaderModule.h"
#include "DescriptorModule.h"
#include "DepthBufferModule.h"
#include "AntiAliasingModule.h"
#include "KeyboardController.h"

class DepthBufferModule;

class GraphicsPipelineModule
{
public:
    VkRenderPass                renderPass;
    VkPipeline                  graphicsPipeline;
    VkPipelineLayout            pipelineLayout;
private:
    DeviceModule*               deviceModule;
    std::vector<ShaderModule*>  shaderModules;
    AntiAliasingModule*         antialias_ptr;
    KeyboardController*         keyboard_ptr;
    VkPolygonMode               PoligonMode;
    
public:
    GraphicsPipelineModule();
    void createRenderPass(VkFormat& swapChainImageFormat, DepthBufferModule& depthBufferModule);
    void createGraphicsPipeline(VkExtent2D& swapChainExtent, VkDescriptorSetLayout& descriptorSetLayout);
    void addShaderModules(ShaderModule& shader_module);
    void addAntialiasingModule(AntiAliasingModule& antialiasingModule);
    void cleanup();
private:
    void hookKeyboardEvents();
    void unhookKeyboardEvents();
    void updatePolygonMode(__int8 polygonType);
};

#endif
