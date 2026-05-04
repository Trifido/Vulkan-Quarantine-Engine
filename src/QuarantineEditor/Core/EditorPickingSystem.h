#pragma once

#include <memory>
#include <vector>
#include <glm/glm.hpp>

class QEGameObject;
class QECamera;
class GameObjectManager;

struct QERay
{
    glm::vec3 Origin{ 0.0f };
    glm::vec3 Direction{ 0.0f, 0.0f, -1.0f };
};

class EditorPickingSystem
{
public:
    explicit EditorPickingSystem();

    std::shared_ptr<QEGameObject> PickGameObject(
        const std::shared_ptr<QECamera>& camera,
        float mouseScreenX,
        float mouseScreenY,
        float viewportScreenX,
        float viewportScreenY,
        float viewportWidth,
        float viewportHeight) const;

private:
    QERay BuildRayFromScreenPoint(
        const std::shared_ptr<QECamera>& camera,
        float mouseScreenX,
        float mouseScreenY,
        float viewportScreenX,
        float viewportScreenY,
        float viewportWidth,
        float viewportHeight) const;

    void CollectCandidates(
        const std::shared_ptr<QEGameObject>& root,
        std::vector<std::shared_ptr<QEGameObject>>& outObjects) const;

    bool IntersectRayAABB(
        const QERay& ray,
        const glm::vec3& aabbMin,
        const glm::vec3& aabbMax,
        float& outTMin,
        float& outTMax) const;

    bool IntersectRayTriangle(
        const QERay& ray,
        const glm::vec3& v0,
        const glm::vec3& v1,
        const glm::vec3& v2,
        float& outT) const;

    QERay TransformRay(
        const QERay& ray,
        const glm::mat4& matrix) const;

    bool PickMeshTriangles(
        const std::shared_ptr<QEGameObject>& gameObject,
        const QERay& worldRay,
        float& outWorldDistance) const;

private:
    GameObjectManager* gameObjectManager = nullptr;
};
