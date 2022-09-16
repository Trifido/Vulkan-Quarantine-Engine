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

struct ComplexVertex : Vertex
{/*
    glm::vec3 Tangents;
    glm::vec3 Bitangents;*/
};

struct PrimitiveVertex : Vertex
{
    glm::vec3 color;
};

#endif
