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

enum class SHADER_TYPE
{
    VERTEX_SHADER = 0,
    FRAGMENT_SHADER = 1,
    GEOMETRY_SHADER = 2,
    TESELLATION_SHADER = 3,
    COMPUTE_SHADER = 4
};

class GraphicsPipelineManager;
class PipelineModule;
class GraphicsPipelineModule;

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
    VkVertexInputBindingDescription                 bindingDescription;
    std::vector<VkVertexInputAttributeDescription>  attributeDescriptions;
    GraphicsPipelineManager*                        graphicsPipelineManager;

public:
    ReflectShader                                   reflectShader;
    std::vector<VkPipelineShaderStageCreateInfo>    shaderStages;
    VkPipelineVertexInputStateCreateInfo            vertexInputInfo{};
    VkDescriptorSetLayout                           descriptorSetLayout;
    std::shared_ptr<GraphicsPipelineModule>         PipelineModule;

public:
    ShaderModule();
    ShaderModule(std::string computeShaderName);
    ShaderModule(std::string vertexShaderName, std::string fragmentShaderName);
    static std::vector<char> readFile(const std::string& filename);
    void createShaderModule(const std::string& filename_compute);
    void createShaderModule(const std::string& filename_vertex, const std::string& filename_fragment);
    void CleanDescriptorSetLayout();
    void cleanup();
private:
    VkPipelineShaderStageCreateInfo createShader(VkDevice& device, const std::string& filename, SHADER_TYPE shaderType);
    void CreateDescriptorSetLayout();
    void createShaderBindings();
};

#endif
