#include <UBO.h>
#include "BufferManageModule.h"

void UniformBufferObject::CreateUniformBuffer(VkDeviceSize bufferSize, uint32_t numImages, DeviceModule& device)
{
    this->uniformBuffers.resize(numImages);
    this->uniformBuffersMemory.resize(numImages);

    for (size_t i = 0; i < numImages; i++)
    {
        BufferManageModule::createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, this->uniformBuffers[i], this->uniformBuffersMemory[i], device);
    }
}


void UniformBufferObject::CreateSSBO(VkDeviceSize bufferSize, uint32_t numImages, DeviceModule& device)
{
    this->uniformBuffers.resize(numImages);
    this->uniformBuffersMemory.resize(numImages);

    for (size_t i = 0; i < numImages; i++)
    {
        BufferManageModule::createBuffer(bufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, this->uniformBuffers[i], this->uniformBuffersMemory[i], device);
    }
}

void UniformBufferObject::FillSSBO(VkBuffer stagingBuffer, VkDeviceSize bufferSize, uint32_t numImages, DeviceModule& device)
{
    for (size_t i = 0; i < numImages; i++)
    {
        BufferManageModule::copyBuffer(stagingBuffer, this->uniformBuffers.at(i), bufferSize, device);
    }
}
