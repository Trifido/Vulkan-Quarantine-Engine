#include "Mesh.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <unordered_map>
#include <stdexcept>

#include "BufferManageModule.h"
#include <Geometry/MeshImporter.h>


Mesh::Mesh(const MeshData& data)
{
    this->PATH = data.name;
    this->numFaces = (uint32_t) data.numFaces;
    this->hasAnimationData = data.HasAnimation;
    this->vertices = data.vertices;
    this->animationData = data.animationData;
    this->numVertices = (uint32_t) data.numVertices;
    this->indices = data.indices;
}

void Mesh::InitializeMesh()
{
    this->meshlets_ptr = std::make_shared<Meshlet>();
    this->meshlets_ptr->GenerateMeshlet(this->vertices, this->indices);
    //this->meshlets_ptr->GenerateCustomMeshlet(this->vertices, this->indices);

    this->createVertexBuffer();

    if (this->hasAnimationData)
    {
        this->createAnimationVertexBuffer();
    }

    this->createIndexBuffer();
}

void Mesh::createVertexBuffer()
{
    VkDeviceSize bufferSize = sizeof(Vertex) * vertices.size();
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    BufferManageModule::createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory, *deviceModule_ptr);

    void* data;
    vkMapMemory(deviceModule_ptr->device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, vertices.data(), (size_t)bufferSize);
    vkUnmapMemory(deviceModule_ptr->device, stagingBufferMemory);

    //Rasterization -> VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
    //RayTracing -> VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
    BufferManageModule::createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory, *deviceModule_ptr);

    BufferManageModule::copyBuffer(stagingBuffer, vertexBuffer, bufferSize, *deviceModule_ptr);

    vkDestroyBuffer(deviceModule_ptr->device, stagingBuffer, nullptr);
    vkFreeMemory(deviceModule_ptr->device, stagingBufferMemory, nullptr);
}

void Mesh::createAnimationVertexBuffer()
{
    VkDeviceSize bufferSize = sizeof(AnimationVertexData) * this->animationData.size();
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    BufferManageModule::createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory, *deviceModule_ptr);

    void* data;
    vkMapMemory(deviceModule_ptr->device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, this->animationData.data(), (size_t)bufferSize);
    vkUnmapMemory(deviceModule_ptr->device, stagingBufferMemory);

    BufferManageModule::createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, animationBuffer, animationBufferMemory, *deviceModule_ptr);

    BufferManageModule::copyBuffer(stagingBuffer, animationBuffer, bufferSize, *deviceModule_ptr);

    vkDestroyBuffer(deviceModule_ptr->device, stagingBuffer, nullptr);
    vkFreeMemory(deviceModule_ptr->device, stagingBufferMemory, nullptr);
}
