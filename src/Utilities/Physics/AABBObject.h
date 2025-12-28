#pragma once

#ifndef AABB_OBJECT_H
#define AABB_OBJECT_H

#include <memory>
#include <vector>
#include <glm/glm.hpp>

#include "QETransform.h"
#include "QEGeometryComponent.h"

class AABBObject : public QEGeometryComponent
{
    REFLECTABLE_DERIVED_COMPONENT(AABBObject, QEGameComponent)
private:
    std::shared_ptr<QETransform> transform;

public:
    glm::vec3 min;
    glm::vec3 max;
    glm::vec3 Size;
    glm::vec3 Center;
    bool isGameObjectVisible;

    std::vector<glm::vec4> vertices;
    std::vector<uint32_t> indices;

private:
    void CreateVertexBuffers();
    void CreateIndexBuffers();

public:
    AABBObject();
    void CreateBuffers();
    void CleanResources();
    void AddTransform(std::shared_ptr<QETransform> modelTransform);
    const std::shared_ptr<QETransform> GetTransform();

    bool IsSerializable() const override { return false; }
    void QEStart() override;
    void QEInit() override;
    void QEUpdate() override;
    void QEDestroy() override;
};

#endif // !AABB_OBJECT_H
