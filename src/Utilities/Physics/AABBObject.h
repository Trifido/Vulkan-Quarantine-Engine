#pragma once

#ifndef AABB_OBJECT_H
#define AABB_OBJECT_H

#include <memory>
#include <vector>
#include <glm/glm.hpp>

#include "Transform.h"
#include "QEGeometryComponent.h"

class AABBObject : public QEGeometryComponent
{
private:
    std::shared_ptr<Transform> transform;

public:
    glm::vec3 min;
    glm::vec3 max;
    glm::vec3 Size;
    glm::vec3 Center;
    bool isGameObjectVisible = true;
    std::vector<glm::vec4> vertices;
    std::vector<uint32_t> indices;

private:
    void CreateVertexBuffers();
    void CreateIndexBuffers();

public:
    AABBObject() = default;
    void CreateBuffers();
    void CleanResources();
    void AddTransform(std::shared_ptr<Transform> modelTransform);
    const std::shared_ptr<Transform> GetTransform();
};

#endif // !AABB_OBJECT_H
