#pragma once

#ifndef VERTEX_H
#define VERTEX_H

#include <glm/glm.hpp>

struct Vertex
{
    glm::vec4 Position;
    glm::vec4 Normal;
    alignas(16)glm::vec2 UV;
    bool operator==(const Vertex& other) const {
        return this->Position == other.Position && this->Normal == other.Normal;
    }
    glm::vec4 Tangent;
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

struct DebugVertex
{
    glm::vec4 pos;
    glm::vec4 col;

    DebugVertex(glm::vec4 position, glm::vec4 color) { pos = position; col = color; }
};

#endif
