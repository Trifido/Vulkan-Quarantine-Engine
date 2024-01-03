#include "CullingSceneManager.h"

CullingSceneManager* CullingSceneManager::instance = nullptr;

CullingSceneManager::CullingSceneManager()
{
    this->aabb_objects.reserve(32);
}

CullingSceneManager* CullingSceneManager::getInstance()
{
    if (instance == NULL)
        instance = new CullingSceneManager();

    return instance;
}

std::shared_ptr<AABBObject> CullingSceneManager::GenerateAABB(std::pair<glm::vec3, glm::vec3> aabbData)
{
    AABBObject aabb{
        .min = aabbData.first,
        .max = aabbData.second,
        .Size = (aabbData.second + aabbData.first) * 0.5f,
        .Center = (aabbData.first - aabbData.second) * 0.5f,
    };

    this->aabb_objects.push_back(std::make_shared<AABBObject>(aabb));

    return this->aabb_objects.back();
}
