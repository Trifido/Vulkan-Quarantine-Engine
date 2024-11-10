#include "ComputePipelineModule.h"

void ComputePipelineModule::CompileComputePipeline(std::vector<VkPipelineShaderStageCreateInfo> shaderInfo, std::vector<VkDescriptorSetLayout> descriptorLayouts)
{
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = descriptorLayouts.size(); // Number of descriptor sets
    pipelineLayoutInfo.pSetLayouts = descriptorLayouts.data(); // Ptr to descriptor set layout

    if (vkCreatePipelineLayout(deviceModule->device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create compute pipeline layout!");
    }

    VkComputePipelineCreateInfo computePipelineCreateInfo = {};
    computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    computePipelineCreateInfo.pNext = nullptr;
    computePipelineCreateInfo.flags = 0;
    computePipelineCreateInfo.layout = pipelineLayout;
    computePipelineCreateInfo.stage = shaderInfo.front();
    computePipelineCreateInfo.basePipelineHandle = 0;
    computePipelineCreateInfo.basePipelineIndex = 0;

    if (vkCreateComputePipelines(deviceModule->device, VK_NULL_HANDLE, 1, &computePipelineCreateInfo, nullptr, &pipeline) != VK_SUCCESS) {
        throw std::runtime_error("failed to create compute pipeline!");
    }
}

void ComputePipelineModule::cleanup(VkPipeline pipeline, VkPipelineLayout pipelineLayout)
{
    vkDestroyPipeline(deviceModule->device, pipeline, nullptr);
    vkDestroyPipelineLayout(deviceModule->device, pipelineLayout, nullptr);
}
