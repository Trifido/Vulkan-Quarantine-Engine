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
    this->numAttributes = 3;
    this->numVertices = data.numPositions;
    this->numFaces = data.numFaces;
    this->vertices = data.vertices;
    this->indices = data.indices;
}

void Mesh::InitializeMesh()
{
    this->numAttributes = 5;
    this->createVertexBuffer();
    this->createIndexBuffer();
}

void Mesh::createVertexBuffer()
{
    VkDeviceSize bufferSize = sizeof(PBRVertex) * vertices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    BufferManageModule::createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory, *deviceModule_ptr);

    void* data;
    vkMapMemory(deviceModule_ptr->device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, vertices.data(), (size_t)bufferSize);
    vkUnmapMemory(deviceModule_ptr->device, stagingBufferMemory);

    //Rasterization -> VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
    //RayTracing -> VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
    BufferManageModule::createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory, *deviceModule_ptr);

    BufferManageModule::copyBuffer(stagingBuffer, vertexBuffer, bufferSize, *deviceModule_ptr);

    vkDestroyBuffer(deviceModule_ptr->device, stagingBuffer, nullptr);
    vkFreeMemory(deviceModule_ptr->device, stagingBufferMemory, nullptr);
}
