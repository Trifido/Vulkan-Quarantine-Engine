#pragma once

#ifndef AABB_OBJECT_H
#define AABB_OBJECT_H

#include <glm/glm.hpp>

class AABBObject
{
public:
    glm::vec3 min;
    glm::vec3 max;
    glm::vec3 Size;
    glm::vec3 Center;
    bool isGameObjectVisible = true;
    bool isAABBVisible = false;
};

#endif // !AABB_OBJECT_H
