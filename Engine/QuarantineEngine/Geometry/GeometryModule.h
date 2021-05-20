#pragma once
#pragma once
#ifndef GEOMETRY_MODULE_H
#define GEOMETRY_MODULE_H

#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>
#include <vector>
#include <array>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 texCoord;

    bool operator==(const Vertex& other) const {
        return pos == other.pos && color == other.color && texCoord == other.texCoord;
    }
};

namespace std {
    template<> struct hash<Vertex> {
        size_t operator()(Vertex const& vertex) const {
            return ((hash<glm::vec3>()(vertex.pos) ^
                (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
                (hash<glm::vec2>()(vertex.texCoord) << 1);
        }
    };
}

struct UniformBufferObject {    //CAMERA
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

class GeometryModule
{
public:
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    unsigned int numVertices;
    unsigned int numFaces;
public:
    static VkVertexInputBindingDescription  getBindingDescription();
    static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions();
};

#endif
