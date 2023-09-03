#pragma once
#ifndef SHADER_MODULE_H
#define SHADER_MODULE_H

#include <vector>
#include <string>
#include <vulkan/vulkan.hpp>
#include "DeviceModule.h"
#include "GeometryComponent.h"
#include <ReflectShader.h>
#include <Numbered.h>
#include <GraphicsPipelineManager.h>
#include <ComputePipelineManager.h>

enum class SHADER_TYPE
{
    VERTEX_SHADER = 0,
    FRAGMENT_SHADER = 1,
    GEOMETRY_SHADER = 2,
    TESELLATION_SHADER = 3,
    COMPUTE_SHADER = 4
};

class GraphicsPipelineManager;
class ComputePipelineManager;
class PipelineModule;
class GraphicsPipelineModule;
class ComputePipelineModule;

struct GraphicsPipelineData
{
    VkPolygonMode polygonMode = VkPolygonMode::VK_POLYGON_MODE_FILL;
    uint32_t vertexBufferStride = sizeof(PBRVertex);

    GraphicsPipelineData() {}
};

class ShaderModule : public Numbered
{
private:
    DeviceModule*                                   deviceModule;
    VkShaderModule                                  vertex_shader = nullptr;
    VkPipelineShaderStageCreateInfo                 vertShaderStageInfo{};
    VkShaderModule                                  fragment_shader = nullptr;
    VkPipelineShaderStageCreateInfo                 fragShaderStageInfo{};
    VkShaderModule                                  compute_shader = nullptr;
    VkPipelineShaderStageCreateInfo                 compShaderStageInfo{};
    std::shared_ptr<VkVertexInputBindingDescription>   bindingDescription;
    std::vector<VkVertexInputAttributeDescription>  attributeDescriptions;
    GraphicsPipelineManager*                        graphicsPipelineManager;
    ComputePipelineManager*                         computePipelineManager;
    GraphicsPipelineData                            graphicsPipelineData;

public:
    ReflectShader                                   reflectShader;
    std::vector<VkPipelineShaderStageCreateInfo>    shaderStages;
    VkPipelineVertexInputStateCreateInfo            vertexInputInfo{};
    VkDescriptorSetLayout                           descriptorSetLayout;
    std::shared_ptr<GraphicsPipelineModule>         PipelineModule;
    std::shared_ptr<ComputePipelineModule>          ComputePipelineModule;

public:
    ShaderModule();
    ShaderModule(std::string computeShaderName);
    ShaderModule(std::string vertexShaderName, std::string fragmentShaderName, GraphicsPipelineData pipelineData = GraphicsPipelineData());

    static std::vector<char> readFile(const std::string& filename);
    void createShaderModule(const std::string& filename_compute);
    void createShaderModule(const std::string& filename_vertex, const std::string& filename_fragment);
    void CleanDescriptorSetLayout();
    void cleanup();
    void CleanLastResources();
    void RecreatePipeline();
private:
    VkPipelineShaderStageCreateInfo createShader(VkDevice& device, const std::string& filename, SHADER_TYPE shaderType);
    void CreateDescriptorSetLayout();
    void createShaderBindings();
    void SetBindingDescription();
    void SetAttributeDescriptions(std::vector<VkVertexInputAttributeDescription>& attributeDescriptions);
};

#endif
