#include "AABBObject.h"
#include <BufferManageModule.h>

void AABBObject::CreateVertexBuffers()
{
    vertexBuffer.resize(1);
    vertexBufferMemory.resize(1);

    VkDeviceSize bufferSize = sizeof(glm::vec4) * vertices.size();

    if (bufferSize == 0)
    {
        return;
    }
    VkBufferUsageFlags usageFlags = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    CreateGeometryBuffer(bufferSize, usageFlags, vertices.data(), vertexBuffer[0], vertexBufferMemory[0]);
}

void AABBObject::CreateIndexBuffers()
{
    indexBuffer.resize(1);
    indexBufferMemory.resize(1);

    VkDeviceSize bufferSize = sizeof(uint32_t) * indices.size();

    if (bufferSize == 0)
    {
        return;
    }

    VkBufferUsageFlags usageFlags = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    CreateGeometryBuffer(bufferSize, usageFlags, indices.data(), indexBuffer[0], indexBufferMemory[0]);
}

AABBObject::AABBObject()
{
    isGameObjectVisible = true;
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

    this->CreateVertexBuffers();
    this->CreateIndexBuffers();
}

void AABBObject::CleanResources()
{
    this->transform = nullptr;
    this->vertices.clear();
    this->indices.clear();

    QEDestroy();
}

void AABBObject::AddTransform(std::shared_ptr<QETransform> modelTransform)
{
    this->transform = modelTransform;
}

const std::shared_ptr<QETransform> AABBObject::GetTransform()
{
    return this->transform;
}

void AABBObject::QEStart()
{
    if (_QEStarted)
    {
        return;
    }

    QEGameComponent::QEStart();

    QEGameComponent::QEInit();
}

void AABBObject::QEInit()
{
    if (_QEInitialized)
    {
        return;
    }

    QEGameComponent::QEInit();
}

void AABBObject::QEUpdate()
{
}

void AABBObject::QEDestroy()
{
    if (_QEDestroyed)
    {
        return;
    }

    QEGeometryComponent::QEDestroy();
}
