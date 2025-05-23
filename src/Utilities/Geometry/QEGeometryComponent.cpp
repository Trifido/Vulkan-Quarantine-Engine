#include "QEGeometryComponent.h"
#include "BufferManageModule.h"

DeviceModule* QEGeometryComponent::deviceModule_ptr;

void QEGeometryComponent::cleanup()
{
    for(int i = 0; i < indexBufferMemory.size(); i++)
    {
        if (indexBufferMemory[i] != VK_NULL_HANDLE)
        {
            vkDestroyBuffer(deviceModule_ptr->device, indexBuffer[i], nullptr);
            vkFreeMemory(deviceModule_ptr->device, indexBufferMemory[i], nullptr);
        }
    }

    for(int i = 0; i < vertexBufferMemory.size(); i++)
    {
        if (vertexBufferMemory[i] != VK_NULL_HANDLE)
        {
            vkDestroyBuffer(deviceModule_ptr->device, vertexBuffer[i], nullptr);
            vkFreeMemory(deviceModule_ptr->device, vertexBufferMemory[i], nullptr);
        }
    }

    for(int i = 0; i < animationBufferMemory.size(); i++)
    {
        if (animationBufferMemory[i] != VK_NULL_HANDLE)
        {
            vkDestroyBuffer(deviceModule_ptr->device, animationBuffer[i], nullptr);
            vkFreeMemory(deviceModule_ptr->device, animationBufferMemory[i], nullptr);
        }
    }
}

void QEGeometryComponent::QEStart()
{
}

void QEGeometryComponent::QEUpdate()
{
}

void QEGeometryComponent::QERelease()
{
}

void QEGeometryComponent::BuildMesh()
{
    mesh = generator->GenerateQEMesh();
    CreateIndexBuffers();
    CreateVertexBuffers();
    CreateAnimationVertexBuffers();
    CreateMeshlets();
}

void QEGeometryComponent::CreateMeshlets()
{
    meshlets_ptr.resize(mesh.MeshData.size());
    for (int i = 0; i < mesh.MeshData.size(); i++)
    {
        meshlets_ptr[i] = std::make_shared<Meshlet>();
        meshlets_ptr[i]->GenerateMeshlet(mesh.MeshData[i].Vertices, mesh.MeshData[i].Indices);
    }
}

void QEGeometryComponent::CreateIndexBuffers()
{
    indexBuffer.resize(mesh.MeshData.size());
    indexBufferMemory.resize(mesh.MeshData.size());
    for (int i = 0; i < mesh.MeshData.size(); i++)
    {
        VkDeviceSize bufferSize = sizeof(mesh.MeshData[i].Indices[0]) * mesh.MeshData[i].Indices.size();

        if (bufferSize == 0)
        {
            return;
        }

        VkBufferUsageFlags usageFlags = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
        CreateGeometryBuffer(bufferSize, usageFlags, mesh.MeshData[i].Indices.data(), indexBuffer[i], indexBufferMemory[i]);
    }
}

void QEGeometryComponent::CreateVertexBuffers()
{
    vertexBuffer.resize(mesh.MeshData.size());
    vertexBufferMemory.resize(mesh.MeshData.size());
    for (int i = 0; i < mesh.MeshData.size(); i++)
    {
        VkDeviceSize bufferSize = sizeof(mesh.MeshData[i].Vertices[0]) * mesh.MeshData[i].Vertices.size();
        if (bufferSize == 0)
        {
            return;
        }
        VkBufferUsageFlags usageFlags = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
        CreateGeometryBuffer(bufferSize, usageFlags, mesh.MeshData[i].Vertices.data(), vertexBuffer[i], vertexBufferMemory[i]);
    }
}

void QEGeometryComponent::CreateAnimationVertexBuffers()
{
    animationBuffer.resize(mesh.MeshData.size());
    animationBufferMemory.resize(mesh.MeshData.size());
    for (int i = 0; i < mesh.MeshData.size(); i++)
    {
        VkDeviceSize bufferSize = sizeof(mesh.MeshData[i].AnimationData[0]) * mesh.MeshData[i].AnimationData.size();
        if (bufferSize == 0)
        {
            return;
        }
        VkBufferUsageFlags usageFlags = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        CreateGeometryBuffer(bufferSize, usageFlags, mesh.MeshData[i].AnimationData.data(), animationBuffer[i], animationBufferMemory[i]);
    }
}

void QEGeometryComponent::CreateGeometryBuffer(VkDeviceSize bufferSize, VkBufferUsageFlags usageFlags, const void* dataArray, VkBuffer& buffer, VkDeviceMemory& memory)
{
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    BufferManageModule::createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory, *deviceModule_ptr);

    void* data;
    vkMapMemory(deviceModule_ptr->device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, dataArray, (size_t)bufferSize);
    vkUnmapMemory(deviceModule_ptr->device, stagingBufferMemory);

    BufferManageModule::createBuffer(bufferSize, usageFlags, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, buffer, memory, *deviceModule_ptr);
    BufferManageModule::copyBuffer(stagingBuffer, buffer, bufferSize, *deviceModule_ptr);

    vkDestroyBuffer(deviceModule_ptr->device, stagingBuffer, nullptr);
    vkFreeMemory(deviceModule_ptr->device, stagingBufferMemory, nullptr);
}
