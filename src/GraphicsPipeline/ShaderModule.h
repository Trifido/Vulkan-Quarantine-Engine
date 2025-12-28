#pragma once
#ifndef SHADER_MODULE_H
#define SHADER_MODULE_H

#include <vector>
#include <string>
#include <vulkan/vulkan.hpp>
#include "DeviceModule.h"
#include "QEGeometryComponent.h"
#include <ReflectShader.h>
#include <Numbered.h>
#include <GraphicsPipelineManager.h>
#include <ComputePipelineManager.h>
#include <ShadowPipelineManager.h>
#include <GraphicsPipelineData.h>

enum class SHADER_TYPE
{
    VERTEX_SHADER = 0,
    FRAGMENT_SHADER = 1,
    GEOMETRY_SHADER = 2,
    TESELLATION_SHADER = 3,
    COMPUTE_SHADER = 4,
    TASK_SHADER = 5,
    MESH_SHADER = 6
};

class GraphicsPipelineManager;
class ComputePipelineManager;
class ShadowPipelineManager;
class PipelineModule;
class GraphicsPipelineModule;
class ComputePipelineModule;
class ShadowPipelineModule;

class ShaderModule : public Numbered
{
private:
    DeviceModule*                                   deviceModule;
    VkShaderModule                                  vertex_shader = nullptr;
    VkPipelineShaderStageCreateInfo                 vertShaderStageInfo{};
    VkShaderModule                                  fragment_shader = nullptr;
    VkPipelineShaderStageCreateInfo                 fragShaderStageInfo{};
    VkShaderModule                                  geometry_shader = nullptr;
    VkPipelineShaderStageCreateInfo                 geoShaderStageInfo{};
    VkShaderModule                                  compute_shader = nullptr;
    VkPipelineShaderStageCreateInfo                 compShaderStageInfo{};
    VkShaderModule                                  task_shader = nullptr;
    VkPipelineShaderStageCreateInfo                 taskShaderStageInfo{};
    VkShaderModule                                  mesh_shader = nullptr;
    VkPipelineShaderStageCreateInfo                 meshShaderStageInfo{};

    std::shared_ptr<VkVertexInputBindingDescription>   bindingDescription;
    std::vector<VkVertexInputAttributeDescription>  attributeDescriptions;
    GraphicsPipelineManager*                        graphicsPipelineManager;
    ComputePipelineManager*                         computePipelineManager;
    ShadowPipelineManager*                          shadowPipelineManager;
    GraphicsPipelineData                            graphicsPipelineData;

public:
    std::string                                     shaderNameID;
    ReflectShader                                   reflectShader;
    std::vector<VkPipelineShaderStageCreateInfo>    shaderStages;
    VkPipelineVertexInputStateCreateInfo            vertexInputInfo{};
    std::vector<VkDescriptorSetLayout>              descriptorSetLayouts;
    std::shared_ptr<GraphicsPipelineModule>         PipelineModule;
    std::shared_ptr<ComputePipelineModule>          ComputePipelineModule;
    std::shared_ptr<ShadowPipelineModule>           ShadowPipelineModule;
    bool                                            IsShadowShader = false;

public:
    ShaderModule(std::string shaderId);
    ShaderModule(std::string shaderId, std::string shaderName, GraphicsPipelineData pipelineData = GraphicsPipelineData());
    ShaderModule(std::string shaderId, std::string vertexShaderName, std::string fragmentShaderName, GraphicsPipelineData pipelineData = GraphicsPipelineData());
    ShaderModule(std::string shaderId, std::string firstShaderName, std::string secondShaderName, std::string thirdShaderName, GraphicsPipelineData pipelineData = GraphicsPipelineData());

    static std::vector<char> readFile(const std::string& filename);
    void createShaderModule(const std::string& filename_compute);
    void createShadowShaderModule(const std::string& filename_compute);
    void createShadowShaderModule(const std::string& filename_vertex, const std::string& filename_fragment);
    void createShaderModule(const std::string& filename_vertex, const std::string& filename_fragment);
    void createShaderModule(const std::string& filename_vertex, const std::string& filename_geometry, const std::string& filename_fragment);
    void createMeshShaderModule(const std::string& filename_task, const std::string& filename_mesh, const std::string& filename_fragment);
    void CleanDescriptorSetLayout();
    void cleanup();
    void CleanLastResources();
    void RecreatePipeline();
private:
    VkPipelineShaderStageCreateInfo createShader(VkDevice& device, const std::string& filename, SHADER_TYPE shaderType);
    void CreateDescriptorSetLayout();
    void CreateShaderBindings();
    void SetBindingDescription();
    void SetAttributeDescriptions(std::vector<VkVertexInputAttributeDescription>& attributeDescriptions);
};

#endif
