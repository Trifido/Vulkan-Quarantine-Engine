#pragma once

#ifndef GAEOMETRYCOMPONENT_H
#define GAEOMETRYCOMPONENT_H

#include "GameComponent.h"

struct Vertex {
    glm::vec3 pos;
    glm::vec3 norm;
    glm::vec3 color;
    glm::vec3 Tangents;
    glm::vec3 Bitangents;
    glm::vec2 texCoord;

    bool operator==(const Vertex& other) const {
        return pos == other.pos /*&& color == other.color*/ && texCoord == other.texCoord && norm == other.norm;
    }
};

class GeometryComponent : GameComponent
{
public:
    GeometryComponent() {}
};

#endif
