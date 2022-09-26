#include "Mesh.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <unordered_map>
#include <stdexcept>

#include "BufferManageModule.h"
#include <Geometry/MeshImporter.h>


Mesh::Mesh()
{

}

Mesh::Mesh(std::string pathfile)
{
    MeshImporter importer = {};
    MeshData data = importer.LoadMesh(pathfile);

    this->PATH = pathfile;

    this->numAttributes = 3;
    this->numVertices = data.numPositions;
    this->numFaces = data.numFaces;
    this->vertices = data.vertices;
    this->indices = data.indices;
}

VkVertexInputBindingDescription Mesh::getBindingDescription()
{
    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(PBRVertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return bindingDescription;
}

std::vector<VkVertexInputAttributeDescription> Mesh::getAttributeDescriptions() {
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};
    attributeDescriptions.resize(this->numAttributes);

    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(PBRVertex, pos);

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(PBRVertex, norm);

    attributeDescriptions[2].binding = 0;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[2].offset = offsetof(PBRVertex, texCoord);

    attributeDescriptions[3].binding = 0;
    attributeDescriptions[3].location = 3;
    attributeDescriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[3].offset = offsetof(PBRVertex, Tangents);

    attributeDescriptions[4].binding = 0;
    attributeDescriptions[4].location = 4;
    attributeDescriptions[4].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[4].offset = offsetof(PBRVertex, Bitangents);

    return attributeDescriptions;
}

void Mesh::InitializeMesh()
{
    //tinyobj::attrib_t attrib;
    //std::vector<tinyobj::shape_t> shapes;
    //std::vector<tinyobj::material_t> materials;
    //std::string warn, err;

    //if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, this->PATH.c_str())) {
    //    throw std::runtime_error(warn + err);
    //}

    //std::unordered_map<Vertex, uint32_t> uniqueVertices{};

    //for (const auto& shape : shapes) {
    //    for (const auto& index : shape.mesh.indices) {
    //        PBRVertex vertex{};

    //        vertex.pos = {
    //            attrib.vertices[3 * index.vertex_index + 0],
    //            attrib.vertices[3 * index.vertex_index + 1],
    //            attrib.vertices[3 * index.vertex_index + 2]
    //        };

    //        if (!attrib.normals.empty())
    //        {
    //            vertex.norm = {
    //                attrib.normals[3 * index.normal_index + 0],
    //                attrib.normals[3 * index.normal_index + 1],
    //                attrib.normals[3 * index.normal_index + 2]
    //            };
    //        }
    //        else
    //        {
    //            vertex.norm = { 1.0f, 1.0f, 1.0f };
    //        }

    //        if (!attrib.texcoords.empty())
    //        {
    //            vertex.texCoord = {
    //                attrib.texcoords[2 * index.texcoord_index + 0],
    //                1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
    //            };
    //        }
    //        else
    //        {
    //            vertex.texCoord = { 0.0f, 0.0f };
    //        }

    //        if (uniqueVertices.count(vertex) == 0) {
    //            uniqueVertices[vertex] = static_cast<uint32_t>(this->vertices.size());
    //            this->vertices.push_back(vertex);
    //        }

    //        this->indices.push_back(uniqueVertices[vertex]);
    //    }
    //}

    this->numAttributes = 5;

    //this->numVertices = (uint32_t)this->indices.size();
    //this->numFaces = this->numVertices / 3;

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
