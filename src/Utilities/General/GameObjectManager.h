#pragma once

#ifndef GAME_OBJECT_MANAGER_H
#define GAME_OBJECT_MANAGER_H

#include <unordered_map>
#include <memory>
#include "QEGameObject.h"
#include "RenderLayerModule.h"
#include "QESingleton.h"
#include <fstream>
#include <vector>

class GameObjectManager : public QESingleton<GameObjectManager>
{
private:
    friend class QESingleton<GameObjectManager>;
    std::unordered_map<unsigned int, std::unordered_map<std::string, std::shared_ptr<QEGameObject>>> _objects;
    RenderLayerModule renderLayers;

private:
    std::string CheckName(std::string nameGameObject);
    unsigned int DecideRenderLayer(std::shared_ptr<QEGameObject> go, unsigned int defaultLayer);
    void RegisterSingle(std::shared_ptr<QEGameObject> go, std::string name);

    void UnregisterSingle(const std::shared_ptr<QEGameObject>& go);
    void UnregisterHierarchy(const std::shared_ptr<QEGameObject>& go);
    void DestroyHierarchy(const std::shared_ptr<QEGameObject>& go);

public:
    GameObjectManager() = default;

    void AddGameObject(std::shared_ptr<QEGameObject> object_ptr);
    bool RemoveGameObject(const std::shared_ptr<QEGameObject>& object_ptr);
    std::shared_ptr<QEGameObject> CreateEmptyGameObject(const std::string& baseName = "Empty GameObject");

    std::shared_ptr<QEGameObject> GetGameObject(const std::string& name);
    void DrawCommand(VkCommandBuffer& commandBuffer, uint32_t idx);
    void CSMCommand(VkCommandBuffer& commandBuffer, uint32_t idx, VkPipelineLayout pipelineLayout, uint32_t cascadeIndex);
    void OmniShadowCommand(VkCommandBuffer& commandBuffer, uint32_t idx, VkPipelineLayout pipelineLayout, glm::mat4 viewParameter, glm::vec3 lightPosition);
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
};

template<typename T>
std::shared_ptr<T> GameObjectManager::FindFirstComponentInScene(const std::string& excludedGameObjectName) const
{
    for (unsigned int idl = 0; idl < this->renderLayers.GetCount(); ++idl)
    {
        const unsigned int layerId = this->renderLayers.GetLayer(idl);
        auto layerIt = this->_objects.find(layerId);
        if (layerIt == this->_objects.end())
            continue;

        for (const auto& kv : layerIt->second)
        {
            const auto& go = kv.second;
            if (!go)
                continue;

            if (!excludedGameObjectName.empty() && go->Name == excludedGameObjectName)
                continue;

            auto component = go->GetComponent<T>();
            if (component)
                return component;
        }
    }

    return nullptr;
}

#endif
