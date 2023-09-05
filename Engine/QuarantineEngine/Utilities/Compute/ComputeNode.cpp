#include "ComputeNode.h"
#include "SynchronizationModule.h"
#include "BufferManageModule.h"

ComputeNode::ComputeNode()
{
    this->deviceModule = DeviceModule::getInstance();
    //this->shaderStorageBuffers = std::make_shared<std::vector<VkBuffer>>();
    //this->shaderStorageBuffersMemory = std::make_shared<std::vector<VkDeviceMemory>>();
}

ComputeNode::ComputeNode(std::string computeShaderPath) : ComputeNode(std::make_shared<ShaderModule>(computeShaderPath))
{
}

ComputeNode::ComputeNode(std::shared_ptr<ShaderModule> shader_ptr) : ComputeNode()
{
    this->computeShader = shader_ptr;
    this->computeDescriptor = std::make_shared<ComputeDescriptorBuffer>(this->computeShader);
}

void ComputeNode::FillComputeBuffer(size_t numElements, unsigned long long elementType, void* data)
{
    this->nElements = numElements;

    VkDeviceSize bufferSize = elementType * this->nElements;

    // Create a staging buffer used to upload data to the gpu
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    BufferManageModule::createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer, stagingBufferMemory, *deviceModule);

    void* auxData;
    vkMapMemory(deviceModule->device, stagingBufferMemory, 0, bufferSize, 0, &auxData);
    memcpy(auxData, data, (size_t)bufferSize);
    vkUnmapMemory(deviceModule->device, stagingBufferMemory);

    // Initialize ssbo
    this->computeDescriptor->ssbo->CreateSSBO(bufferSize, MAX_FRAMES_IN_FLIGHT, *deviceModule);
    this->computeDescriptor->ssboSize = bufferSize;
    // Fill ssbo
    this->computeDescriptor->ssbo->FillSSBO(stagingBuffer, bufferSize, MAX_FRAMES_IN_FLIGHT, *deviceModule);

    vkDestroyBuffer(deviceModule->device, stagingBuffer, nullptr);
    vkFreeMemory(deviceModule->device, stagingBufferMemory, nullptr);
}

void ComputeNode::InitializeComputeNode()
{
    this->computeDescriptor->InitializeDescriptorSets(this->computeShader);
}

void ComputeNode::cleanup()
{
    //for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    //{
    //    vkDestroyBuffer(deviceModule->device, shaderStorageBuffers->at(i), nullptr);
    //    vkFreeMemory(deviceModule->device, shaderStorageBuffersMemory->at(i), nullptr);
    //}
}

void ComputeNode::DispatchCommandBuffer(VkCommandBuffer commandBuffer, uint32_t currentFrame)
{
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, this->computeShader->ComputePipelineModule->pipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, this->computeShader->ComputePipelineModule->pipelineLayout, 0, 1, &computeDescriptor->descriptorSets[currentFrame], 0, 0);

    uint32_t groupX = (this->nElements < 256) ? this->nElements : this->nElements / 256;
    vkCmdDispatch(commandBuffer, groupX, 1, 1);
}

void ComputeNode::UpdateComputeDescriptor()
{
    this->computeDescriptor->UpdateUBODeltaTime();
}
