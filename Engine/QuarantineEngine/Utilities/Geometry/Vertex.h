#pragma once

#ifndef VERTEX_H
#define VERTEX_H

#include <glm/glm.hpp>

struct Vertex
{
    glm::vec4 pos;
    glm::vec4 norm;
    alignas(16)glm::vec2 texCoord;
    bool operator==(const Vertex& other) const {
        return this->pos == other.pos && this->norm == other.norm;
    }
};

struct PBRVertex : Vertex
{
    glm::vec4 Tangents;
    glm::vec4 Bitangents;
    int boneIDs[4];
    float boneWeights[4];
};

struct PBRAnimationVertex
{
    glm::vec4 pos;
    glm::vec4 norm;
    alignas(16)glm::vec2 texCoord;
    glm::vec4 Tangents;
    glm::vec4 Bitangents;
    int boneIDs[4];
    float boneWeights[4];

    bool operator==(const Vertex& other) const {
        return this->pos == other.pos && this->norm == other.norm;
    }
};

struct PrimitiveVertex : Vertex
{
    glm::vec4 color;
};

struct Particle
{
    glm::vec3 position;
    float lifeTime;
    glm::vec4 color;
    glm::vec3 velocity;
    float seed;
};

struct AnimationVertex
{
    glm::vec4 pos;
    glm::vec4 norm;
    alignas(16)glm::vec2 texCoord;
    glm::vec4 tangent;
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
