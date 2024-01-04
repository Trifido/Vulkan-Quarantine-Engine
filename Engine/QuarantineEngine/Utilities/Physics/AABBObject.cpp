#include "AABBObject.h"
#include <BufferManageModule.h>

void AABBObject::createVertexBuffer()
{
    VkDeviceSize bufferSize;

    bufferSize = sizeof(glm::vec4) * vertices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    BufferManageModule::createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory, *deviceModule_ptr);

    void* data;
    vkMapMemory(deviceModule_ptr->device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, vertices.data(), (size_t)bufferSize);
    vkUnmapMemory(deviceModule_ptr->device, stagingBufferMemory);

    BufferManageModule::createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory, *deviceModule_ptr);

    BufferManageModule::copyBuffer(stagingBuffer, vertexBuffer, bufferSize, *deviceModule_ptr);

    vkDestroyBuffer(deviceModule_ptr->device, stagingBuffer, nullptr);
    vkFreeMemory(deviceModule_ptr->device, stagingBufferMemory, nullptr);
}

void AABBObject::CreateBuffers()
{
    this->vertices = {
        glm::vec4(min, 1.0f) , glm::vec4(max.x, min.y, min.z, 1.0f), glm::vec4(max.x, min.y, max.z, 1.0f), glm::vec4(min.x, min.y, max.z, 1.0f),
        glm::vec4(min.x, max.y, min.z, 1.0f), glm::vec4(max.x, max.y, min.z, 1.0f), glm::vec4(max, 1.0f), glm::vec4(min.x, max.y, max.z, 1.0f)
    };

    this->indices = {
    0, 1, 1, 2, 2, 3, 3, 0, 0, 4, 4, 5, 5, 6, 6, 7, 7, 4, 3, 7, 2, 6, 1, 5
    };

    this->numFaces = 12;
    this->numVertices = 8;

    size_t numAttributes = 1;
    this->InitializeMesh(numAttributes);
}

void AABBObject::CleanResources()
{
    this->transform = nullptr;
    this->vertices.clear();
    this->indices.clear();

    this->cleanup();
}

void AABBObject::InitializeMesh(size_t numAttributes)
{
    this->numAttributes = numAttributes;
    this->createVertexBuffer();
    this->createIndexBuffer();
}
