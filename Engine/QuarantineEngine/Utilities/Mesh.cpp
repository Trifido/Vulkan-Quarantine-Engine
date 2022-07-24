#include "Mesh.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#include <unordered_map>
#include <stdexcept>

void Mesh::loadMesh(std::string pathfile)
{
    path = pathfile;
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.c_str())) {
        throw std::runtime_error(warn + err);
    }

    std::unordered_map<Vertex, uint32_t> uniqueVertices{};

    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            Vertex vertex{};

            vertex.pos = {
                attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2]
            };

            if (!attrib.normals.empty())
            {
                vertex.norm = {
                    attrib.normals[3 * index.normal_index + 0],
                    attrib.normals[3 * index.normal_index + 1],
                    attrib.normals[3 * index.normal_index + 2]
                };
            }
            else
            {
                vertex.norm = { 1.0f, 1.0f, 1.0f };
            }

            vertex.texCoord = {
                attrib.texcoords[2 * index.texcoord_index + 0],
                1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
            };

            //vertex.color = { 1.0f, 1.0f, 1.0f };

            if (uniqueVertices.count(vertex) == 0) {
                uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
                vertices.push_back(vertex);
            }

            indices.push_back(uniqueVertices[vertex]);
        }
    }

    numVertices = (uint32_t)indices.size();
    numFaces = numVertices / 3;

    createVertexBuffer();
    createIndexBuffer();
}

VkVertexInputBindingDescription Mesh::getBindingDescription()
{
    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(Vertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return bindingDescription;
}

std::array<VkVertexInputAttributeDescription, 3> Mesh::getAttributeDescriptions() {
    std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(Vertex, pos);

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(Vertex, norm);

    //attributeDescriptions[1].binding = 0;
    //attributeDescriptions[1].location = 2;
    //attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    //attributeDescriptions[1].offset = offsetof(Vertex, color);

    attributeDescriptions[2].binding = 0;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

    return attributeDescriptions;
}

Mesh::Mesh(DeviceModule& deviceModule, VkCommandPool& commandPool, QueueModule& queueModule)
{
    deviceModule_ptr = &deviceModule;
    commandPool_ptr = &commandPool;
    queueModule_ptr = &queueModule;
}

void Mesh::cleanup()
{
    vkDestroyBuffer(deviceModule_ptr->device, indexBuffer, nullptr);
    vkFreeMemory(deviceModule_ptr->device, indexBufferMemory, nullptr);
    vkDestroyBuffer(deviceModule_ptr->device, vertexBuffer, nullptr);
    vkFreeMemory(deviceModule_ptr->device, vertexBufferMemory, nullptr);
}

void Mesh::createVertexBuffer()
{
    VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

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

    BufferManageModule::copyBuffer(stagingBuffer, vertexBuffer, bufferSize, *commandPool_ptr, *queueModule_ptr, *deviceModule_ptr);

    vkDestroyBuffer(deviceModule_ptr->device, stagingBuffer, nullptr);
    vkFreeMemory(deviceModule_ptr->device, stagingBufferMemory, nullptr);
}

void Mesh::createIndexBuffer()
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

    BufferManageModule::copyBuffer(stagingBuffer, indexBuffer, bufferSize, *commandPool_ptr, *queueModule_ptr, *deviceModule_ptr);

    vkDestroyBuffer(deviceModule_ptr->device, stagingBuffer, nullptr);
    vkFreeMemory(deviceModule_ptr->device, stagingBufferMemory, nullptr);
}
