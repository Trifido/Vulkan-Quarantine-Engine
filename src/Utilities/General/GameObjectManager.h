#pragma once

#ifndef GAME_OBJECT_MANAGER_H
#define GAME_OBJECT_MANAGER_H

#include <unordered_map>
#include <memory>
#include "QEGameObject.h"
#include "RenderLayerModule.h"
#include "QESingleton.h"
#include <fstream>

class GameObjectManager : public QESingleton<GameObjectManager>
{
private:
    friend class QESingleton<GameObjectManager>;
    std::unordered_map<unsigned int, std::unordered_map<std::string, std::shared_ptr<QEGameObject>>> _objects;
    std::unordered_map<std::string, std::shared_ptr<QEGameObject>> _physicObjects;
    RenderLayerModule renderLayers;

private:
    std::string CheckName(std::string nameGameObject);

public:
    GameObjectManager() = default;
    void AddGameObject(std::shared_ptr<QEGameObject> object_ptr, std::string name);
    std::shared_ptr<QEGameObject> GetGameObject(std::string name);
    void DrawCommand(VkCommandBuffer& commandBuffer, uint32_t idx);
    void CSMCommand(VkCommandBuffer& commandBuffer, uint32_t idx, VkPipelineLayout pipelineLayout, uint32_t cascadeIndex);
    void OmniShadowCommand(VkCommandBuffer& commandBuffer, uint32_t idx, VkPipelineLayout pipelineLayout, glm::mat4 viewParameter, glm::vec3 lightPosition);
    void ReleaseAllGameObjects();
    void CleanLastResources();
    std::vector<GameObjectDto> GetGameObjectDtos(std::ifstream& file);
    void SaveGameObjects(std::ofstream& file);
    void LoadGameObjectDtos(std::vector<GameObjectDto>& gameObjectDtos);

    void StartQEGameObjects();
    void UpdateQEGameObjects();
    void DestroyQEGameObjects();
};

#endif
