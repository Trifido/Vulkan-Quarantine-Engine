#pragma once

#ifndef CULLING_CENE_MANAGER_H
#define CULLING_CENE_MANAGER_H

#include <vector>
#include <AABBObject.h>
#include <Camera.h>

class CullingSceneManager
{
private:
    std::vector<std::shared_ptr<AABBObject> > aabb_objects;
    std::shared_ptr<Camera> camera;

public:
    static CullingSceneManager* instance;

public:
    CullingSceneManager();
    static CullingSceneManager* getInstance();
    std::shared_ptr<AABBObject> GenerateAABB(std::pair<glm::vec3, glm::vec3> aabbData);
};

#endif // !CULLING_CENE_MANAGER_H


