#pragma once

#ifndef VERTEX_H
#define VERTEX_H

#include <glm/glm.hpp>

struct Vertex
{
    glm::vec3 pos;
    glm::vec3 norm;
    glm::vec2 texCoord;
    bool operator==(const Vertex& other) const {
        return this->pos == other.pos && this->norm == other.norm;
    }
};

struct PBRVertex : Vertex
{
    glm::vec3 Tangents;
    glm::vec3 Bitangents;
    int boneIDs[4];
    float boneWeights[4];
};

struct PrimitiveVertex : Vertex
{
    glm::vec3 color;
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

#endif
