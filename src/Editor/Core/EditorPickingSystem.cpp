#include "EditorPickingSystem.h"

#include <limits>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <GameObjectManager.h>
#include <QEGameObject.h>
#include <QECamera.h>
#include <QETransform.h>
#include <QEGeometryComponent.h>
#include <AABBObject.h>

EditorPickingSystem::EditorPickingSystem()
{
    gameObjectManager = GameObjectManager::getInstance();
}

std::shared_ptr<QEGameObject> EditorPickingSystem::PickGameObject(
    std::shared_ptr<QECamera> camera,
    float mouseScreenX,
    float mouseScreenY,
    float viewportScreenX,
    float viewportScreenY,
    float viewportWidth,
    float viewportHeight) const
{
    if (!gameObjectManager || !camera)
        return nullptr;

    if (viewportWidth <= 0.0f || viewportHeight <= 0.0f)
        return nullptr;

    Ray ray = BuildRayFromScreenPoint(
        camera,
        mouseScreenX,
        mouseScreenY,
        viewportScreenX,
        viewportScreenY,
        viewportWidth,
        viewportHeight);

    std::vector<std::shared_ptr<QEGameObject>> candidates;
    const auto roots = gameObjectManager->GetRootGameObjects();

    for (const auto& root : roots)
    {
        CollectCandidates(root, candidates);
    }

    std::shared_ptr<QEGameObject> closestObject = nullptr;
    float closestDistance = std::numeric_limits<float>::max();

    for (const auto& gameObject : candidates)
    {
        if (!gameObject)
            continue;

        auto transform = gameObject->GetComponent<QETransform>();
        auto aabbObject = gameObject->GetComponent<AABBObject>();

        if (!transform || !aabbObject)
            continue;

        // Aproximación inicial:
        // centro = world position
        // radio = half diagonal del bounding box local escalado aprox
        const glm::vec3 localMin = aabbObject->min;
        const glm::vec3 localMax = aabbObject->max;
        const glm::vec3 localCenter = aabbObject->Center;
        const glm::vec3 localExtent = aabbObject->Size * 0.5f;

        glm::vec4 worldCenter4 = transform->GetWorldMatrix() * glm::vec4(localCenter, 1.0f);
        glm::vec3 worldCenter(worldCenter4);

        glm::vec3 worldScale = transform->GetWorldScale();
        float maxScale = glm::max(worldScale.x, glm::max(worldScale.y, worldScale.z));
        float radius = glm::length(localExtent) * maxScale;

        float distance = 0.0f;
        if (IntersectRaySphere(ray, worldCenter, radius, distance))
        {
            if (distance < closestDistance)
            {
                closestDistance = distance;
                closestObject = gameObject;
            }
        }
    }

    return closestObject;
}

Ray EditorPickingSystem::BuildRayFromScreenPoint(
    std::shared_ptr<QECamera> camera,
    float mouseScreenX,
    float mouseScreenY,
    float viewportScreenX,
    float viewportScreenY,
    float viewportWidth,
    float viewportHeight) const
{
    Ray ray{};

    const float localX = mouseScreenX - viewportScreenX;
    const float localY = mouseScreenY - viewportScreenY;

    const float ndcX = (2.0f * localX) / viewportWidth - 1.0f;
    const float ndcY = 1.0f - (2.0f * localY) / viewportHeight;

    glm::vec4 nearClip(ndcX, ndcY, 0.0f, 1.0f);
    glm::vec4 farClip(ndcX, ndcY, 1.0f, 1.0f);

    glm::mat4 invViewProj = glm::inverse(camera->CameraData->Projection * camera->CameraData->View);

    glm::vec4 nearWorld = invViewProj * nearClip;
    glm::vec4 farWorld = invViewProj * farClip;

    nearWorld /= nearWorld.w;
    farWorld /= farWorld.w;

    ray.Origin = glm::vec3(nearWorld);
    ray.Direction = glm::normalize(glm::vec3(farWorld - nearWorld));

    return ray;
}

void EditorPickingSystem::CollectCandidates(
    const std::shared_ptr<QEGameObject>& root,
    std::vector<std::shared_ptr<QEGameObject>>& outObjects) const
{
    if (!root)
        return;

    outObjects.push_back(root);

    for (const auto& child : root->childs)
    {
        CollectCandidates(child, outObjects);
    }
}

bool EditorPickingSystem::IntersectRaySphere(
    const Ray& ray,
    const glm::vec3& center,
    float radius,
    float& outDistance) const
{
    const glm::vec3 oc = ray.Origin - center;

    const float a = glm::dot(ray.Direction, ray.Direction);
    const float b = 2.0f * glm::dot(oc, ray.Direction);
    const float c = glm::dot(oc, oc) - radius * radius;

    const float discriminant = b * b - 4.0f * a * c;
    if (discriminant < 0.0f)
        return false;

    const float sqrtD = sqrtf(discriminant);
    const float t0 = (-b - sqrtD) / (2.0f * a);
    const float t1 = (-b + sqrtD) / (2.0f * a);

    float t = t0;
    if (t < 0.0f)
        t = t1;

    if (t < 0.0f)
        return false;

    outDistance = t;
    return true;
}
