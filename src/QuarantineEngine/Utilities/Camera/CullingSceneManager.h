#pragma once

#ifndef CULLING_CENE_MANAGER_H
#define CULLING_CENE_MANAGER_H

#include <vector>
#include <FrustumComponent.h>
#include "QETransform.h"
#include <Material.h>
#include <QESingleton.h>

class CullingSceneManager : public QESingleton<CullingSceneManager>
{
private:
    friend class QESingleton<CullingSceneManager>;

    std::vector<std::shared_ptr<AABBObject>> aabb_objects;
    std::shared_ptr<ShaderModule> shader_aabb_ptr = nullptr;
    std::shared_ptr<QEMaterial> material_aabb_ptr = nullptr;

public:
    bool DebugMode = false;

public:
    void EnsureInitialized();
    void ResetSceneState();

    std::shared_ptr<AABBObject> GenerateAABB(std::pair<glm::vec3, glm::vec3> aabbData, std::shared_ptr<QETransform> transform_ptr);
    void DrawDebug(VkCommandBuffer& commandBuffer, uint32_t idx);
    void UpdateCullingScene();

private:
    void InitializeCullingSceneResources();
    void CleanUp();
};



namespace QE
{
    using ::CullingSceneManager;
} // namespace QE
// QE namespace aliases
#endif
