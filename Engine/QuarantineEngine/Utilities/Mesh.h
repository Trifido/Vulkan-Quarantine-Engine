#pragma once
#ifndef MESH_H
#define MESH_H

#include <vulkan/vulkan.hpp>
#include <string>

#include "GameComponent.h"
#include "DeviceModule.h"
#include "QueueModule.h"
#include "BufferManageModule.h"

struct Vertex {
    glm::vec3 pos;
    glm::vec3 norm;
    //glm::vec3 color;
    glm::vec2 texCoord;

    bool operator==(const Vertex& other) const {
        return pos == other.pos /*&& color == other.color*/ && texCoord == other.texCoord && norm == other.norm;
    }
};

namespace std {
    template<> struct hash<Vertex> {
        size_t operator()(Vertex const& vertex) const {
            return ((hash<glm::vec3>()(vertex.pos) ^
                (hash<glm::vec3>()(vertex.norm) << 1)) >> 1) ^
                (hash<glm::vec2>()(vertex.texCoord) << 1);
        }
    };
}

class Mesh : public GameComponent
{
private:
    DeviceModule*       deviceModule_ptr;
    VkCommandPool*      commandPool_ptr;
    QueueModule*        queueModule_ptr;
    VkDeviceMemory      vertexBufferMemory;
    VkDeviceMemory      indexBufferMemory;
public:
    VkBuffer vertexBuffer;
    VkBuffer indexBuffer;
    std::string path;
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    uint32_t numVertices;
    uint32_t numFaces;

private:
    void createVertexBuffer();
    void createIndexBuffer();
public:
    Mesh(DeviceModule& deviceModule, VkCommandPool& commandPool, QueueModule& queueModule);
    void loadMesh(std::string pathfile);
    VkVertexInputBindingDescription  getBindingDescription();
    std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions();
    void cleanup();
};

#endif // !MESH_H
