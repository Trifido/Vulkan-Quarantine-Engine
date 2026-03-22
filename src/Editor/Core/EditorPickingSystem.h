#pragma once

#include <memory>
#include <vector>
#include <glm/glm.hpp>

class QEGameObject;
class QECamera;
class GameObjectManager;

struct Ray
{
    glm::vec3 Origin{ 0.0f };
    glm::vec3 Direction{ 0.0f, 0.0f, -1.0f };
};

class EditorPickingSystem
{
public:
    explicit EditorPickingSystem();

    std::shared_ptr<QEGameObject> PickGameObject(
        std::shared_ptr<QECamera> camera,
        float mouseScreenX,
        float mouseScreenY,
        float viewportScreenX,
        float viewportScreenY,
        float viewportWidth,
        float viewportHeight) const;

private:
    Ray BuildRayFromScreenPoint(
        std::shared_ptr<QECamera> camera,
        float mouseScreenX,
        float mouseScreenY,
        float viewportScreenX,
        float viewportScreenY,
        float viewportWidth,
        float viewportHeight) const;

    void CollectCandidates(
        const std::shared_ptr<QEGameObject>& root,
        std::vector<std::shared_ptr<QEGameObject>>& outObjects) const;

    bool IntersectRaySphere(
        const Ray& ray,
        const glm::vec3& center,
        float radius,
        float& outDistance) const;

private:
    GameObjectManager* gameObjectManager = nullptr;
};
