#pragma once

#ifndef AABB_OBJECT_H
#define AABB_OBJECT_H

#include <GeometryComponent.h>
#include <Transform.h>
#include <glm/glm.hpp>
#include <vector>

class AABBObject : public GeometryComponent
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

private:
    void createVertexBuffer() override;

public:
    void CreateBuffers();
    void CleanResources();
    void InitializeMesh() override;
    void AddTransform(std::shared_ptr<Transform> modelTransform);
    const std::shared_ptr<Transform> GetTransform();
};

#endif // !AABB_OBJECT_H
