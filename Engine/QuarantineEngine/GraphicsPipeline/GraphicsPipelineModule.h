#pragma once
#ifndef GRAPHICS_PIPELINE_MODULE_H
#define GRAPHICS_PIPELINE_MODULE_H

#include "GraphicsPipeline.h"
#include "KeyboardController.h"

//class DepthBufferModule;

class GraphicsPipelineModule : IObserver
{
public:
    GraphicsPipeline* gp_current = nullptr;
private:
    GraphicsPipeline* gp_fill = nullptr;
    GraphicsPipeline* gp_line = nullptr;
    GraphicsPipeline* gp_point = nullptr;
    KeyboardController* keyboard_ptr = nullptr;
    std::shared_ptr<ShaderModule> shaderModule_ptr;
public:
    GraphicsPipelineModule();
    void Initialize(AntiAliasingModule& AAModule, std::shared_ptr<ShaderModule> SModule, SwapChainModule& SCModule, DepthBufferModule& DBModule, std::shared_ptr<DescriptorModule> DModule);
    void cleanup();
    void Update(const __int8& message_from_subject);
private:
    void updatePolygonMode(__int8 polygonType);
};

#endif
