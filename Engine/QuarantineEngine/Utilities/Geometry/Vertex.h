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
    int boneIDs[4];
    float boneWeights[4];
};

struct PBRAnimationVertex
{
    glm::vec4 pos;
    glm::vec4 norm;
    alignas(16)glm::vec2 texCoord;
    glm::vec4 Tangents;
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
    float angle;
    glm::vec4 auxiliarData;
    glm::vec2 currentOffset;
    glm::vec2 nextOffset;
};

struct AnimationVertex
{
    glm::vec4 pos;
    glm::vec4 norm;
    alignas(16)glm::vec2 texCoord;
    glm::vec4 tangent;
};

struct VertexMeshlet
{
    glm::vec4 pos;

    VertexMeshlet()
    {

    }

    VertexMeshlet(PBRVertex vertex)
    {
        this->pos.x = vertex.pos.x;
        this->pos.y = vertex.pos.y;
        this->pos.z = vertex.pos.z;
        this->pos.w = vertex.pos.w;
    }
};

#endif
