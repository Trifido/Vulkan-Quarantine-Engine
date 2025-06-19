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
    REFLECTABLE_DERIVED_COMPONENT(AABBObject, QEGameComponent)
private:
    std::shared_ptr<Transform> transform;

public:
    REFLECT_PROPERTY(glm::vec3, min)
    REFLECT_PROPERTY(glm::vec3, max)
    REFLECT_PROPERTY(glm::vec3, Size)
    REFLECT_PROPERTY(glm::vec3, Center)
    REFLECT_PROPERTY(bool, isGameObjectVisible)

    std::vector<glm::vec4> vertices;
    std::vector<uint32_t> indices;

private:
    void CreateVertexBuffers();
    void CreateIndexBuffers();

public:
    AABBObject();
    void CreateBuffers();
    void CleanResources();
    void AddTransform(std::shared_ptr<Transform> modelTransform);
    const std::shared_ptr<Transform> GetTransform();
};

#endif // !AABB_OBJECT_H
