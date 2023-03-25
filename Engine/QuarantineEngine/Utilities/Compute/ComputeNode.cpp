#include "ComputeNode.h"
#include "SynchronizationModule.h"
#include "BufferManageModule.h"

ComputeNode::ComputeNode()
{
    this->deviceModule = DeviceModule::getInstance();
    this->shaderStorageBuffers = std::make_shared<std::vector<VkBuffer>>();
    this->shaderStorageBuffersMemory = std::make_shared<std::vector<VkDeviceMemory>>();
}

ComputeNode::ComputeNode(std::shared_ptr<ShaderModule> shader_ptr)
{
    this->deviceModule = DeviceModule::getInstance();
    this->shaderStorageBuffers = std::make_shared<std::vector<VkBuffer>>();
    this->shaderStorageBuffersMemory = std::make_shared<std::vector<VkDeviceMemory>>();
    this->computeShader = shader_ptr;
}

void ComputeNode::AddComputePipeline(std::shared_ptr<ComputePipelineModule> computePipelineModule_ptr)
{
    this->computePipelineModule = computePipelineModule_ptr;
}

void ComputeNode::FillComputeBuffer(size_t numElements, unsigned long long elementType, void* data)
{
    this->bufferSize = elementType * numElements;

    // Create a staging buffer used to upload data to the gpu
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    BufferManageModule::createBuffer(this->bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer, stagingBufferMemory, *deviceModule);

    void* auxData;
    vkMapMemory(deviceModule->device, stagingBufferMemory, 0, this->bufferSize, 0, &auxData);
    memcpy(auxData, data, (size_t)this->bufferSize);
    vkUnmapMemory(deviceModule->device, stagingBufferMemory);

    shaderStorageBuffers->resize(MAX_FRAMES_IN_FLIGHT);
    shaderStorageBuffersMemory->resize(MAX_FRAMES_IN_FLIGHT);

    // Copy initial particle data to all storage buffers
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        BufferManageModule::createBuffer(this->bufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, shaderStorageBuffers->at(i), shaderStorageBuffersMemory->at(i), *deviceModule);
        BufferManageModule::copyBuffer(stagingBuffer, shaderStorageBuffers->at(i), this->bufferSize, *deviceModule);
    }

    vkDestroyBuffer(deviceModule->device, stagingBuffer, nullptr);
    vkFreeMemory(deviceModule->device, stagingBufferMemory, nullptr);
}

void ComputeNode::InitializeDescriptor()
{
    this->computeDescriptor = std::make_shared<ComputeDescriptorModule>();
}

void ComputeNode::InitializeComputeNode()
{
    if (this->computePipelineModule != nullptr)
    {
        this->InitializeDescriptor();
        this->computeDescriptor->initializeDescriptor(this->shaderStorageBuffers, this->bufferSize);
        this->computePipelineModule->CreateComputePipeline(this->computePipeline, this->computePipelineLayout, this->computeShader, this->computeDescriptor);
    }
}

void ComputeNode::cleanup()
{
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        vkDestroyBuffer(deviceModule->device, shaderStorageBuffers->at(i), nullptr);
        vkFreeMemory(deviceModule->device, shaderStorageBuffersMemory->at(i), nullptr);
    }
}
