#include "GeometryComponent.h"
#include "BufferManageModule.h"

DeviceModule* GeometryComponent::deviceModule_ptr;

void GeometryComponent::cleanup()
{
    if (this->indexBufferMemory != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(deviceModule_ptr->device, indexBuffer, nullptr);
        vkFreeMemory(deviceModule_ptr->device, indexBufferMemory, nullptr);
    }

    if (this->vertexBufferMemory != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(deviceModule_ptr->device, vertexBuffer, nullptr);
        vkFreeMemory(deviceModule_ptr->device, vertexBufferMemory, nullptr);
    }

    if (this->animationBufferMemory != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(deviceModule_ptr->device, animationBuffer, nullptr);
        vkFreeMemory(deviceModule_ptr->device, animationBufferMemory, nullptr);
    }
}

void GeometryComponent::createIndexBuffer()
{
    VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    BufferManageModule::createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory, *deviceModule_ptr);

    void* data;
    vkMapMemory(deviceModule_ptr->device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, indices.data(), (size_t)bufferSize);
    vkUnmapMemory(deviceModule_ptr->device, stagingBufferMemory);

    //Rasterization -> VK_BUFFER_USAGE_INDEX_BUFFER_BIT
    //RayTracing -> VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
    BufferManageModule::createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory, *deviceModule_ptr);

    BufferManageModule::copyBuffer(stagingBuffer, indexBuffer, bufferSize, *deviceModule_ptr);

    vkDestroyBuffer(deviceModule_ptr->device, stagingBuffer, nullptr);
    vkFreeMemory(deviceModule_ptr->device, stagingBufferMemory, nullptr);
}
