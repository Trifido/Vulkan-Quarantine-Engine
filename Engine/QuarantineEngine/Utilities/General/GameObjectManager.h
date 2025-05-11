#pragma once

#ifndef GAME_OBJECT_MANAGER_H
#define GAME_OBJECT_MANAGER_H

#include <unordered_map>
#include <memory>
#include "GameObject.h"
#include "RenderLayerModule.h"
#include "QESingleton.h"
#include <fstream>

class GameObjectManager : public QESingleton<GameObjectManager>
{
private:
    friend class QESingleton<GameObjectManager>;
    std::unordered_map<unsigned int, std::unordered_map<std::string, std::shared_ptr<GameObject>>> _objects;
    std::unordered_map<std::string, std::shared_ptr<GameObject>> _physicObjects;
    RenderLayerModule renderLayers;

private:
    std::string CheckName(std::string nameGameObject);

public:
    GameObjectManager() = default;
    void AddGameObject(std::shared_ptr<GameObject> object_ptr, std::string name);
    std::shared_ptr<GameObject> GetGameObject(std::string name);
    void DrawCommand(VkCommandBuffer& commandBuffer, uint32_t idx);
    void CSMCommand(VkCommandBuffer& commandBuffer, uint32_t idx, VkPipelineLayout pipelineLayout, uint32_t cascadeIndex);
    void OmniShadowCommand(VkCommandBuffer& commandBuffer, uint32_t idx, VkPipelineLayout pipelineLayout, glm::mat4 viewParameter, glm::vec3 lightPosition);
    void InitializePhysics();
    void UpdatePhysicTransforms();
    void Cleanup();
    void CleanLastResources();
    std::vector<GameObjectDto> GetGameObjectDtos(std::ifstream& file);
    void SaveGameObjects(std::ofstream& file);
    void LoadGameObjectDtos(std::vector<GameObjectDto>& gameObjectDtos);
};

#endif
