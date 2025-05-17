#include "PipelineModule.h"

PipelineModule::PipelineModule()
{
    this->deviceModule = DeviceModule::getInstance();
}

PipelineModule::~PipelineModule()
{
    this->deviceModule = nullptr;
}

void PipelineModule::CompileComputePipeline(std::vector<VkPipelineShaderStageCreateInfo> shaderInfo, std::vector<VkDescriptorSetLayout> descriptorLayouts)
{
}

void PipelineModule::CompileGraphicsPipeline(std::vector<VkPipelineShaderStageCreateInfo> shaderInfo, VkPipelineVertexInputStateCreateInfo vertexInfo, std::vector<VkDescriptorSetLayout> descriptorLayouts)
{
}

void PipelineModule::CompileShadowPipeline(std::vector<VkPipelineShaderStageCreateInfo> shaderInfo, VkPipelineVertexInputStateCreateInfo vertexInfo, std::vector<VkDescriptorSetLayout> descriptorLayouts)
{
}

void PipelineModule::CleanPipelineData()
{
    if (this->pipeline != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(deviceModule->device, this->pipeline, nullptr);
        this->pipeline = VK_NULL_HANDLE;
    }

    if (this->pipelineLayout != VK_NULL_HANDLE)
    {
        vkDestroyPipelineLayout(deviceModule->device, this->pipelineLayout, nullptr);
        this->pipelineLayout = VK_NULL_HANDLE;
    }
}
