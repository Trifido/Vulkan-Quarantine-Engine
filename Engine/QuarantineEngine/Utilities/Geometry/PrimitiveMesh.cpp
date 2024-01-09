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

void PrimitiveMesh::InitializeMesh(size_t numAttributes)
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
    case PRIMITIVE_TYPE::PLANE_TYPE:
        this->InitializeFloorPlane();
        break;
    case PRIMITIVE_TYPE::SPHERE_TYPE:
        this->InitializeSphere();
        break;
    case PRIMITIVE_TYPE::GRID_TYPE:
        this->InitializeGrid();
        break;
    default:
        break;
    }

    this->numAttributes = numAttributes;

    this->numVertices = (uint32_t)this->indices.size();
    this->numFaces = this->numVertices / 3;

    this->createVertexBuffer();
    this->createIndexBuffer();
}

void PrimitiveMesh::InitializePoint()
{
    this->vertices.resize(1);
    PBRVertex vert;

    vert.pos = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    vert.texCoord = glm::vec2(0.0f, 0.0f);
    vert.norm = glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
    vert.Tangents = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
    this->vertices[0] = vert;

    this->indices.resize(1);
    this->indices = { 0 };
}

void PrimitiveMesh::InitializeTriangle()
{
    this->vertices.resize(3);
    PBRVertex vert;

    vert.pos = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
    vert.texCoord = glm::vec2(1.0f, 1.0f);
    this->vertices[0] = vert;

    vert.pos = glm::vec4(-1.0f, 0.0f, 0.0f, 1.0f);
    vert.texCoord = glm::vec2(0.0f, 0.0f);
    this->vertices[1] = vert;

    vert.pos = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
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

    vert.pos = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);
    vert.texCoord = glm::vec2(1.0f, 1.0f);
    this->vertices[0] = vert;

    vert.pos = glm::vec4(-1.0f, -1.0f, 0.0f, 1.0f);
    vert.texCoord = glm::vec2(0.0f, 0.0f);
    this->vertices[1] = vert;

    vert.pos = glm::vec4(-1.0f, 1.0f, 0.0f, 1.0f);
    vert.texCoord = glm::vec2(1.0f, 0.0f);
    this->vertices[2] = vert;

    vert.pos = glm::vec4(1.0f, -1.0f, 0.0f, 1.0f);
    vert.texCoord = glm::vec2(0.0f, 1.0f);
    this->vertices[3] = vert;

    this->indices.resize(6);
    this->indices = { 0, 2, 1, 0, 1, 3 };

    MeshImporter::RecreateNormals(this->vertices, this->indices);
    MeshImporter::RecreateTangents(this->vertices, this->indices);
}

void PrimitiveMesh::InitializeFloorPlane()
{
    this->vertices.resize(4);
    PBRVertex vert;

    vert.pos = glm::vec4(1.0f, 0.0f, 1.0f, 1.0f);
    vert.texCoord = glm::vec2(1.0f, 1.0f);
    this->vertices[0] = vert;

    vert.pos = glm::vec4(-1.0f, 0.0f, -1.0f, 1.0f);
    vert.texCoord = glm::vec2(0.0f, 0.0f);
    this->vertices[1] = vert;

    vert.pos = glm::vec4(-1.0f, 0.0f, 1.0f, 1.0f);
    vert.texCoord = glm::vec2(1.0f, 0.0f);
    this->vertices[2] = vert;

    vert.pos = glm::vec4(1.0f, 0.0f, -1.0f, 1.0f);
    vert.texCoord = glm::vec2(0.0f, 1.0f);
    this->vertices[3] = vert;

    this->indices.resize(6);
    this->indices = { 0, 1, 2, 0, 3, 1 };

    MeshImporter::RecreateNormals(this->vertices, this->indices);
    MeshImporter::RecreateTangents(this->vertices, this->indices);
}

/*
vec3 gridPlane[6] = vec3[] (
    vec3(1, 1, 0), vec3(-1, 1, 0), vec3(-1, -1, 0),
    vec3(1, 1, 0), vec3(-1, -1, 0), vec3(1, -1, 0)
);
*/

void PrimitiveMesh::InitializeGrid()
{
    this->vertices.resize(6);
    PBRVertex vert;

    vert.pos = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);
    vert.texCoord = glm::vec2(1.0f, 1.0f);
    this->vertices[0] = vert;

    vert.pos = glm::vec4(-1.0f, 1.0f, 0.0f, 1.0f);
    vert.texCoord = glm::vec2(0.0f, 1.0f);
    this->vertices[1] = vert;

    vert.pos = glm::vec4(-1.0f, -1.0f, 0.0f, 1.0f);
    vert.texCoord = glm::vec2(0.0f, 0.0f);
    this->vertices[2] = vert;

    vert.pos = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);
    vert.texCoord = glm::vec2(1.0f, 1.0f);
    this->vertices[3] = vert;

    vert.pos = glm::vec4(-1.0f, -1.0f, 0.0f, 1.0f);
    vert.texCoord = glm::vec2(0.0f, 0.0f);
    this->vertices[4] = vert;

    vert.pos = glm::vec4(1.0f, -1.0f, 0.0f, 1.0f);
    vert.texCoord = glm::vec2(1.0f, 0.0f);
    this->vertices[5] = vert;

    this->indices.resize(6);
    this->indices = { 0, 1, 2, 3, 4, 5};

    MeshImporter::RecreateNormals(this->vertices, this->indices);
    MeshImporter::RecreateTangents(this->vertices, this->indices);
}

void PrimitiveMesh::InitializeCube()
{
    MeshData data = MeshImporter::LoadRawMesh(cubevertices, 36, 8);

    this->vertices = data.vertices;
    this->indices = data.indices;

    MeshImporter::RecreateNormals(this->vertices, this->indices);
    MeshImporter::RecreateTangents(this->vertices, this->indices);
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
