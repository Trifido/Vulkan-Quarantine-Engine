#pragma once

#ifndef CULLING_CENE_MANAGER_H
#define CULLING_CENE_MANAGER_H

#include <vector>
#include <FrustumComponent.h>
#include <Transform.h>
#include <Material.h>
#include <QESingleton.h>

class CullingSceneManager : public QESingleton<CullingSceneManager>
{
private:
    friend class QESingleton<CullingSceneManager>; // Permitir acceso al constructor
    std::vector<std::shared_ptr<AABBObject> > aabb_objects;
    std::shared_ptr<ShaderModule> shader_aabb_ptr = nullptr;
    std::shared_ptr<QEMaterial> material_aabb_ptr = nullptr;

public:
    bool DebugMode = true;

public:
    void InitializeCullingSceneResources();
    std::shared_ptr<AABBObject> GenerateAABB(std::pair<glm::vec3, glm::vec3> aabbData, std::shared_ptr<Transform> transform_ptr);
    void CleanUp();
    void DrawDebug(VkCommandBuffer& commandBuffer, uint32_t idx);
    void UpdateCullingScene();
};

#endif // !CULLING_CENE_MANAGER_H


