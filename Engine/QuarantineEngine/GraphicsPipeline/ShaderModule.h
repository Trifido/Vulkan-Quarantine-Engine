#pragma once
#ifndef SHADER_MODULE_H
#define SHADER_MODULE_H

#include <vector>
#include <string>
#include <vulkan/vulkan.hpp>
#include "DeviceModule.h"

enum class SHADER_TYPE
{
    VERTEX_SHADER = 0,
    FRAGMENT_SHADER = 1,
    GEOMETRY_SHADER = 2,
    TESELLATION_SHADER = 3,
    COMPUTE_SHADER = 4
};

class ShaderModule
{
private:
    DeviceModule*                                   deviceModule;
    VkShaderModule                                  vertex_shader;
    VkPipelineShaderStageCreateInfo                 vertShaderStageInfo{};
    VkShaderModule                                  fragment_shader;
    VkPipelineShaderStageCreateInfo                 fragShaderStageInfo{};

    VkVertexInputBindingDescription                     bindingDescription;
    std::array<VkVertexInputAttributeDescription, 3>    attributeDescriptions;
public:
    std::vector<VkPipelineShaderStageCreateInfo>    shaderStages;
    VkPipelineVertexInputStateCreateInfo            vertexInputInfo{};

public:
    ShaderModule();
    static std::vector<char> readFile(const std::string& filename);
    void createShaderModule(const std::string& filename_vertex, const std::string& filename_fragment);
    void cleanup();
private:
    VkPipelineShaderStageCreateInfo createShader(VkDevice& device, const std::string& filename, SHADER_TYPE shaderType);
};

#endif
