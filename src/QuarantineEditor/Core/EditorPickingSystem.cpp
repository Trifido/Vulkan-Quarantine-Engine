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
    const std::shared_ptr<QECamera>& camera,
    float mouseScreenX,
    float mouseScreenY,
    float viewportScreenX,
    float viewportScreenY,
    float viewportWidth,
    float viewportHeight) const
{
    if (!gameObjectManager || !camera || !camera->CameraData)
        return nullptr;

    if (viewportWidth <= 0.0f || viewportHeight <= 0.0f)
        return nullptr;

    const QERay worldRay = BuildRayFromScreenPoint(
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
    float closestWorldDistance = std::numeric_limits<float>::max();

    for (const auto& gameObject : candidates)
    {
        if (!gameObject)
            continue;

        float hitWorldDistance = 0.0f;
        if (PickMeshTriangles(gameObject, worldRay, hitWorldDistance))
        {
            if (hitWorldDistance < closestWorldDistance)
            {
                closestWorldDistance = hitWorldDistance;
                closestObject = gameObject;
            }
        }
    }

    return closestObject;
}

QERay EditorPickingSystem::BuildRayFromScreenPoint(
    const std::shared_ptr<QECamera>& camera,
    float mouseScreenX,
    float mouseScreenY,
    float viewportScreenX,
    float viewportScreenY,
    float viewportWidth,
    float viewportHeight) const
{
    QERay ray{};

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

    for (const auto& child : root->GetChildren())
    {
        CollectCandidates(child, outObjects);
    }
}

bool EditorPickingSystem::IntersectRayAABB(
    const QERay& ray,
    const glm::vec3& aabbMin,
    const glm::vec3& aabbMax,
    float& outTMin,
    float& outTMax) const
{
    const float epsilon = 1e-8f;

    float tMin = 0.0f;
    float tMax = std::numeric_limits<float>::max();

    for (int axis = 0; axis < 3; ++axis)
    {
        const float origin = ray.Origin[axis];
        const float direction = ray.Direction[axis];
        const float minVal = aabbMin[axis];
        const float maxVal = aabbMax[axis];

        if (fabs(direction) < epsilon)
        {
            if (origin < minVal || origin > maxVal)
                return false;
        }
        else
        {
            float invD = 1.0f / direction;
            float t0 = (minVal - origin) * invD;
            float t1 = (maxVal - origin) * invD;

            if (t0 > t1)
                std::swap(t0, t1);

            tMin = std::max(tMin, t0);
            tMax = std::min(tMax, t1);

            if (tMax < tMin)
                return false;
        }
    }

    outTMin = tMin;
    outTMax = tMax;
    return true;
}

bool EditorPickingSystem::IntersectRayTriangle(
    const QERay& ray,
    const glm::vec3& v0,
    const glm::vec3& v1,
    const glm::vec3& v2,
    float& outT) const
{
    const float epsilon = 1e-7f;

    const glm::vec3 edge1 = v1 - v0;
    const glm::vec3 edge2 = v2 - v0;

    const glm::vec3 pvec = glm::cross(ray.Direction, edge2);
    const float det = glm::dot(edge1, pvec);

    if (fabs(det) < epsilon)
        return false;

    const float invDet = 1.0f / det;
    const glm::vec3 tvec = ray.Origin - v0;

    const float u = glm::dot(tvec, pvec) * invDet;
    if (u < 0.0f || u > 1.0f)
        return false;

    const glm::vec3 qvec = glm::cross(tvec, edge1);
    const float v = glm::dot(ray.Direction, qvec) * invDet;
    if (v < 0.0f || (u + v) > 1.0f)
        return false;

    const float t = glm::dot(edge2, qvec) * invDet;
    if (t < 0.0f)
        return false;

    outT = t;
    return true;
}

QERay EditorPickingSystem::TransformRay(
    const QERay& ray,
    const glm::mat4& matrix) const
{
    QERay transformed{};

    const glm::vec4 origin4 = matrix * glm::vec4(ray.Origin, 1.0f);
    const glm::vec4 direction4 = matrix * glm::vec4(ray.Direction, 0.0f);

    transformed.Origin = glm::vec3(origin4);
    transformed.Direction = glm::normalize(glm::vec3(direction4));

    return transformed;
}

bool EditorPickingSystem::PickMeshTriangles(
    const std::shared_ptr<QEGameObject>& gameObject,
    const QERay& worldRay,
    float& outWorldDistance) const
{
    auto transform = gameObject->GetComponent<QETransform>();
    auto geometry = gameObject->GetComponent<QEGeometryComponent>();

    if (!transform || !geometry)
        return false;

    auto mesh = geometry->GetMesh();
    if (!mesh)
        return false;

    const glm::mat4& objectWorldMatrix = transform->GetWorldMatrix();
    const glm::mat4 invObjectWorld = glm::inverse(objectWorldMatrix);
    const QERay objectLocalRay = TransformRay(worldRay, invObjectWorld);

    float closestWorldDistance = std::numeric_limits<float>::max();
    bool hit = false;

    for (const auto& submesh : mesh->MeshData)
    {
        if (submesh.Vertices.empty())
            continue;

        const glm::mat4& submeshModel = submesh.ModelTransform;
        const glm::mat4 invSubmeshModel = glm::inverse(submeshModel);

        const QERay submeshLocalRay = TransformRay(objectLocalRay, invSubmeshModel);

        float aabbTMin = 0.0f;
        float aabbTMax = 0.0f;

        if (!IntersectRayAABB(
            submeshLocalRay,
            submesh.BoundingBox.first,
            submesh.BoundingBox.second,
            aabbTMin,
            aabbTMax))
        {
            continue;
        }

        float closestSubmeshLocalT = std::numeric_limits<float>::max();
        bool submeshHit = false;

        const bool indexed = !submesh.Indices.empty();

        if (indexed)
        {
            const size_t triangleCount = submesh.Indices.size() / 3;

            for (size_t i = 0; i < triangleCount; ++i)
            {
                const uint32_t i0 = submesh.Indices[i * 3 + 0];
                const uint32_t i1 = submesh.Indices[i * 3 + 1];
                const uint32_t i2 = submesh.Indices[i * 3 + 2];

                if (i0 >= submesh.Vertices.size() ||
                    i1 >= submesh.Vertices.size() ||
                    i2 >= submesh.Vertices.size())
                {
                    continue;
                }

                const glm::vec3 v0 = submesh.Vertices[i0].Position;
                const glm::vec3 v1 = submesh.Vertices[i1].Position;
                const glm::vec3 v2 = submesh.Vertices[i2].Position;

                float t = 0.0f;
                if (IntersectRayTriangle(submeshLocalRay, v0, v1, v2, t))
                {
                    if (t < closestSubmeshLocalT)
                    {
                        closestSubmeshLocalT = t;
                        submeshHit = true;
                    }
                }
            }
        }
        else
        {
            const size_t triangleCount = submesh.Vertices.size() / 3;

            for (size_t i = 0; i < triangleCount; ++i)
            {
                const glm::vec3 v0 = submesh.Vertices[i * 3 + 0].Position;
                const glm::vec3 v1 = submesh.Vertices[i * 3 + 1].Position;
                const glm::vec3 v2 = submesh.Vertices[i * 3 + 2].Position;

                float t = 0.0f;
                if (IntersectRayTriangle(submeshLocalRay, v0, v1, v2, t))
                {
                    if (t < closestSubmeshLocalT)
                    {
                        closestSubmeshLocalT = t;
                        submeshHit = true;
                    }
                }
            }
        }

        if (!submeshHit)
            continue;

        const glm::vec3 hitPointSubmeshLocal =
            submeshLocalRay.Origin + submeshLocalRay.Direction * closestSubmeshLocalT;

        const glm::vec3 hitPointObjectLocal =
            glm::vec3(submeshModel * glm::vec4(hitPointSubmeshLocal, 1.0f));

        const glm::vec3 hitPointWorld =
            glm::vec3(objectWorldMatrix * glm::vec4(hitPointObjectLocal, 1.0f));

        const float worldDistance = glm::length(hitPointWorld - worldRay.Origin);

        if (worldDistance < closestWorldDistance)
        {
            closestWorldDistance = worldDistance;
            hit = true;
        }
    }

    if (!hit)
        return false;

    outWorldDistance = closestWorldDistance;
    return true;
}
