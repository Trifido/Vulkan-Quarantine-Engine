#include "PrimitiveMesh.h"
#include <BufferManageModule.h>
#include <MeshImporter.h>
#include "RawGeometry.h"

PrimitiveMesh::PrimitiveMesh()
{
}

PrimitiveMesh::PrimitiveMesh(PRIMITIVE_TYPE type)
{
    this->type = type;
}

VkVertexInputBindingDescription PrimitiveMesh::getBindingDescription()
{
    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(PBRVertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return bindingDescription;
}

std::vector<VkVertexInputAttributeDescription> PrimitiveMesh::getAttributeDescriptions() {
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


void PrimitiveMesh::InitializeMesh()
{
    this->type = type;

    switch (this->type)
    {
    case PRIMITIVE_TYPE::POINT_TYPE:
        this->InitializePoint();
        break;
    case PRIMITIVE_TYPE::TRIANGLE_TYPE:
        this->InitializeTriangle();
        break;
    case PRIMITIVE_TYPE::CUBE_TYPE:
        this->InitializeCube();
        break;
    case PRIMITIVE_TYPE::QUAD_TYPE:
        this->InitializePlane();
        break;
    case PRIMITIVE_TYPE::SPHERE_TYPE:
        this->InitializeSphere();
        break;
    default:
        break;
    }

    this->numAttributes = 5;

    this->numVertices = (uint32_t)this->indices.size();
    this->numFaces = this->numVertices / 3;

    this->createVertexBuffer();
    this->createIndexBuffer();
}

void PrimitiveMesh::InitializePoint()
{
    this->vertices.resize(1);
    PBRVertex vert;

    vert.pos = glm::vec3(0.0f, 0.0f, 0.0f);
    vert.texCoord = glm::vec2(0.0f, 0.0f);
    vert.norm = glm::vec3(0.0f, 1.0f, 0.0f);
    vert.Tangents = glm::vec3(1.0f, 0.0f, 0.0f);
    vert.Bitangents = glm::vec3(0.0f, 0.0f, 1.0f);
    this->vertices[0] = vert;

    this->indices.resize(1);
    this->indices = { 0 };
}

void PrimitiveMesh::InitializeTriangle()
{
    this->vertices.resize(3);
    PBRVertex vert;

    vert.pos = glm::vec3(1.0f, 0.0f, 0.0f);
    vert.texCoord = glm::vec2(1.0f, 1.0f);
    this->vertices[0] = vert;

    vert.pos = glm::vec3(-1.0f, 0.0f, 0.0f);
    vert.texCoord = glm::vec2(0.0f, 0.0f);
    this->vertices[1] = vert;

    vert.pos = glm::vec3(0.0f, 1.0f, 0.0f);
    vert.texCoord = glm::vec2(1.0f, 0.0f);
    this->vertices[2] = vert;

    this->indices.resize(3);
    this->indices = {0, 2, 1};

    MeshImporter::RecreateNormals(this->vertices, this->indices);
    MeshImporter::RecreateTangents(this->vertices, this->indices);
}

void PrimitiveMesh::InitializePlane()
{
    this->vertices.resize(4);
    PBRVertex vert;

    vert.pos = glm::vec3(1.0f, 1.0f, 0.0f);
    vert.texCoord = glm::vec2(1.0f, 1.0f);
    this->vertices[0] = vert;

    vert.pos = glm::vec3(-1.0f, -1.0f, 0.0f);
    vert.texCoord = glm::vec2(0.0f, 0.0f);
    this->vertices[1] = vert;

    vert.pos = glm::vec3(-1.0f, 1.0f, 0.0f);
    vert.texCoord = glm::vec2(1.0f, 0.0f);
    this->vertices[2] = vert;

    vert.pos = glm::vec3(1.0f, -1.0f, 0.0f);
    vert.texCoord = glm::vec2(0.0f, 1.0f);
    this->vertices[3] = vert;

    this->indices.resize(6);
    this->indices = { 0, 2, 1, 0, 1, 3 };

    MeshImporter::RecreateNormals(this->vertices, this->indices);
    MeshImporter::RecreateTangents(this->vertices, this->indices);
}

void PrimitiveMesh::InitializeCube()
{
    MeshData data = MeshImporter::LoadRawMesh(cubevertices, 36, 8);

    this->vertices = data.vertices;
    this->indices = data.indices;
}

void PrimitiveMesh::InitializeSphere()
{
    MeshImporter importer;
    std::vector<MeshData> data = importer.LoadMesh("../../resources/models/Sphere.stl");

    this->vertices = data[0].vertices;
    this->indices = data[0].indices;
}

void PrimitiveMesh::createVertexBuffer()
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
