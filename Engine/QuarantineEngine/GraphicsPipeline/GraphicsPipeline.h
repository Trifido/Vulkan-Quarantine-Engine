#pragma once

#ifndef GRAPHICS_PIPELINE_H
#define GRAPHICS_PIPELINE_H

#include "GraphicsPipelineBase.h"
class GraphicsPipeline : public GraphicsPipelineBase
{
public:
    enum PolygonRenderType
    {
        FILL,
        LINE,
        POINT
    };
private:
    std::vector<std::shared_ptr<ShaderModule> > shaderModules;
    VkPolygonMode               PoligonMode;

public:
    GraphicsPipeline();
    GraphicsPipeline(PolygonRenderType polygonType);
    ~GraphicsPipeline();
    void createGraphicsPipeline(VkExtent2D& swapChainExtent, VkDescriptorSetLayout& descriptorSetLayout) override;
    void addShaderModules(std::shared_ptr<ShaderModule> shader_module) override;
    void cleanup();
private:
    void updatePolygonMode(PolygonRenderType polygonType);
};

#endif
