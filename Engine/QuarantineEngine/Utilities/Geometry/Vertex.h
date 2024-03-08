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
    glm::vec4 Tangents;
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

#endif
