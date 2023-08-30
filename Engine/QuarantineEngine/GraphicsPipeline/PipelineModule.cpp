#include "PipelineModule.h"

PipelineModule::PipelineModule()
{
    this->deviceModule = DeviceModule::getInstance();
}

PipelineModule::~PipelineModule()
{
    this->deviceModule = nullptr;
}

void PipelineModule::CompileComputePipeline(std::vector<VkPipelineShaderStageCreateInfo> shaderInfo, VkDescriptorSetLayout descriptorLayout)
{
}

void PipelineModule::CompileGraphicsPipeline(std::vector<VkPipelineShaderStageCreateInfo> shaderInfo, VkPipelineVertexInputStateCreateInfo vertexInfo, VkDescriptorSetLayout descriptorLayout)
{
}

void PipelineModule::CleanPipelineData()
{
    vkDestroyPipeline(deviceModule->device, this->pipeline, nullptr);
    vkDestroyPipelineLayout(deviceModule->device, this->pipelineLayout, nullptr);
}
