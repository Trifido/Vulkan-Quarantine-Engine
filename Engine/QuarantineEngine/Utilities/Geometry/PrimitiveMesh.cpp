#include "PrimitiveMesh.h"

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
    bindingDescription.stride = sizeof(Vertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return bindingDescription;
}

std::vector<VkVertexInputAttributeDescription> PrimitiveMesh::getAttributeDescriptions() {
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};
    attributeDescriptions.resize(this->numAttributes);

    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(Vertex, pos);

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(Vertex, norm);

    attributeDescriptions[2].binding = 0;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

    return attributeDescriptions;
}


void PrimitiveMesh::InitializeMesh()
{
    this->type = type;

    switch (this->type)
    {
    case PRIMITIVE_TYPE::QUAD_TYPE:
        this->InitializePlane();
        break;
    default:
        break;
    }

    this->numAttributes = 3;

    this->numVertices = (uint32_t)this->indices.size();
    this->numFaces = this->numVertices / 3;

    this->createVertexBuffer();
    this->createIndexBuffer();
}

void PrimitiveMesh::InitializePlane()
{
    this->vertices.resize(4);
    this->vertices[0].pos = glm::vec3(0.0f);
    this->vertices[0].norm = glm::vec3(0.0f, 1.0f, 0.0f);
    this->vertices[0].texCoord = glm::vec2(0.0f, 0.0f);

    this->vertices[1].pos = glm::vec3(1.0f, 0.0f, 0.0f);
    this->vertices[1].norm = glm::vec3(0.0f, 1.0f, 0.0f);
    this->vertices[1].texCoord = glm::vec2(0.0f, 1.0f);

    this->vertices[2].pos = glm::vec3(1.0f, 0.0f, 1.0f);
    this->vertices[2].norm = glm::vec3(0.0f, 1.0f, 0.0f);
    this->vertices[2].texCoord = glm::vec2(1.0f, 1.0f);

    this->vertices[3].pos = glm::vec3(0.0f, 0.0f, 1.0f);
    this->vertices[3].norm = glm::vec3(0.0f, 1.0f, 0.0f);
    this->vertices[3].texCoord = glm::vec2(1.0f, 0.0f);

    this->indices.resize(6);
    this->indices = { 0, 2, 1, 0, 3, 2 };
}
