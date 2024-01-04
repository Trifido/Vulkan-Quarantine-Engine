#pragma once

#ifndef CULLING_CENE_MANAGER_H
#define CULLING_CENE_MANAGER_H

#include <vector>
#include <FrustumComponent.h>
#include <Transform.h>
#include <MaterialManager.h>

class CullingSceneManager
{
private:
    std::vector<std::shared_ptr<AABBObject> > aabb_objects;
    std::shared_ptr<FrustumComponent> cameraFrustum = nullptr;
    std::shared_ptr<ShaderModule> shader_aabb_ptr = nullptr;
    std::shared_ptr<Material> material_aabb_ptr = nullptr;

public:
    static CullingSceneManager* instance;
    bool isDebugEnable = true;

public:
    static CullingSceneManager* getInstance();
    void ResetInstance();
    void AddCameraFrustum(std::shared_ptr<FrustumComponent> frustum);
    void InitializeCullingSceneResources();
    std::shared_ptr<AABBObject> GenerateAABB(std::pair<glm::vec3, glm::vec3> aabbData, std::shared_ptr<Transform> transform_ptr);
    void CleanUp();
    void DrawDebug(VkCommandBuffer& commandBuffer, uint32_t idx);
    void UpdateCullingScene();
};

#endif // !CULLING_CENE_MANAGER_H


