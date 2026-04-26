#pragma once

#ifndef GAME_OBJECT_MANAGER_H
#define GAME_OBJECT_MANAGER_H

#include <unordered_map>
#include <memory>
#include "QEGameObject.h"
#include "QEMeshRenderer.h"
#include "QESingleton.h"
#include <fstream>
#include <vector>

class QELight;
class LightManager;

struct QEOrderRenderItem
{
    std::shared_ptr<QEGameObject> GameObject;
    std::shared_ptr<QEMeshRenderer> MeshRenderer;
    std::shared_ptr<QEMaterial> Material;
    uint32_t SubMeshIndex = 0;
    unsigned int RenderQueue = 0;
    float CameraDistanceSq = 0.0f;
};

class GameObjectManager : public QESingleton<GameObjectManager>
{
private:
    friend class QESingleton<GameObjectManager>;

    std::unordered_map<unsigned int, std::unordered_map<std::string, std::shared_ptr<QEGameObject>>> _objectsByUpdateOrder;

private:
    std::string CheckName(std::string nameGameObject);
    unsigned int DecideUpdateBucket(std::shared_ptr<QEGameObject> go, unsigned int defaultOrder);
    void RegisterSingle(std::shared_ptr<QEGameObject> go, std::string name);

    void UnregisterSingle(const std::shared_ptr<QEGameObject>& go);
    void UnregisterHierarchy(const std::shared_ptr<QEGameObject>& go);
    void DestroyHierarchy(const std::shared_ptr<QEGameObject>& go);

    void RemoveLightsFromHierarchy(const std::shared_ptr<QEGameObject>& go);
    void ReindexLightShadowMaps();

    std::vector<QEOrderRenderItem> BuildRenderItems() const;
    std::vector<QEOrderRenderItem> BuildShadowRenderItems() const;

public:
    GameObjectManager() = default;

    void AddGameObject(std::shared_ptr<QEGameObject> object_ptr);
    bool RemoveGameObject(const std::shared_ptr<QEGameObject>& object_ptr);
    bool RenameGameObject(const std::shared_ptr<QEGameObject>& objectPtr, const std::string& newName);
    bool SetGameObjectUpdateOrder(const std::shared_ptr<QEGameObject>& objectPtr, unsigned int newOrder);

    std::shared_ptr<QEGameObject> CreateEmptyGameObject(const std::string& baseName = "Empty GameObject");

    std::shared_ptr<QEGameObject> GetGameObject(const std::string& name);
    void DrawCommand(VkCommandBuffer& commandBuffer, uint32_t idx);
    void CSMCommand(VkCommandBuffer& commandBuffer, uint32_t idx, VkPipelineLayout pipelineLayout, uint32_t cascadeIndex);
    void OmniShadowCommand(VkCommandBuffer& commandBuffer, uint32_t idx, VkPipelineLayout pipelineLayout, glm::mat4 viewParameter, glm::vec3 lightPosition);

    void ResetSceneState();
    void ReleaseAllGameObjects();
    void CleanLastResources();

    YAML::Node SerializeGameObjects();
    void DeserializeGameObjects(YAML::Node gameObjects);

    void StartQEGameObjects();
    void UpdateQEGameObjects();
    void DestroyQEGameObjects();

    template<typename T>
    std::shared_ptr<T> FindFirstComponentInScene(const std::string& excludedGameObjectName = "") const;

    std::shared_ptr<QEGameComponent> FindGameComponentInScene(const std::string& id);
    std::vector<std::shared_ptr<QEGameObject>> GetRootGameObjects() const;
    std::shared_ptr<QEGameObject> GetGameObjectById(const std::string& id) const;
    void RemoveMaterialReferences(const std::string& materialName);

    void RegisterSceneLights();
};

template<typename T>
std::shared_ptr<T> GameObjectManager::FindFirstComponentInScene(const std::string& excludedGameObjectName) const
{
    for (const auto& bucketPair : _objectsByUpdateOrder)
    {
        const auto& bucket = bucketPair.second;

        for (const auto& kv : bucket)
        {
            const auto& go = kv.second;
            if (!go)
                continue;

            if (!excludedGameObjectName.empty() && go->Name == excludedGameObjectName)
                continue;

            if (!go->IsActiveInHierarchy())
                continue;

            auto component = go->GetComponent<T>();
            if (component)
                return component;
        }
    }

    return nullptr;
}



namespace QE
{
    using ::QELight;
    using ::LightManager;
    using ::QEOrderRenderItem;
    using ::GameObjectManager;
} // namespace QE
// QE namespace aliases
#endif
