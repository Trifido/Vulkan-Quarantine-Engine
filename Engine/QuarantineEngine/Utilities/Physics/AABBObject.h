#pragma once

#ifndef AABB_OBJECT_H
#define AABB_OBJECT_H

#include <GeometryComponent.h>
#include <Transform.h>
#include <glm/glm.hpp>
#include <vector>

class AABBObject : public GeometryComponent
{
public:
    glm::vec3 min;
    glm::vec3 max;
    glm::vec3 Size;
    glm::vec3 Center;
    std::shared_ptr<Transform> transform;
    bool isGameObjectVisible = true;

    std::vector<glm::vec4> vertices;

private:
    void createVertexBuffer() override;

public:
    void CreateBuffers();
    void CleanResources();
    void InitializeMesh(size_t numAttributes) override;
};

#endif // !AABB_OBJECT_H
