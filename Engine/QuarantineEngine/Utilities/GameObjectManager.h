#pragma once

#ifndef GAME_OBJECT_MANAGER_H
#define GAME_OBJECT_MANAGER_H

#include <unordered_map>
#include <memory>
#include "GameObject.h"
#include "RenderLayerModule.h"

class GameObjectManager
{
private:
    std::unordered_map<unsigned int, std::unordered_map<std::string, std::shared_ptr<GameObject>>> _objects;
    std::unordered_map<std::string, std::shared_ptr<GameObject>> _physicObjects;
    RenderLayerModule* renderLayers = nullptr;
public:
    static GameObjectManager* instance;

private:
    std::string CheckName(std::string nameGameObject);

public:
    GameObjectManager();
    void AddGameObject(std::shared_ptr<GameObject> object_ptr, std::string name);
    std::shared_ptr<GameObject> GetGameObject(std::string name);
    static GameObjectManager* getInstance();
    static void ResetInstance();
    void DrawCommand(VkCommandBuffer& commandBuffer, uint32_t idx);
    void CSMCommand(VkCommandBuffer& commandBuffer, uint32_t idx, VkPipelineLayout pipelineLayout, uint32_t cascadeIndex);
    void ShadowCommand(VkCommandBuffer& commandBuffer, uint32_t idx, VkPipelineLayout pipelineLayout);
    void OmniShadowCommand(VkCommandBuffer& commandBuffer, uint32_t idx, VkPipelineLayout pipelineLayout, glm::mat4 viewParameter, glm::vec3 lightPosition);
    void InitializePhysics();
    void UpdatePhysicTransforms();
    void Cleanup();
    void CleanLastResources();
};

#endif
