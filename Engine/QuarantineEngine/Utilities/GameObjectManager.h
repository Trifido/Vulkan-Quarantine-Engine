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

public:
    GameObjectManager();
    void AddGameObject(std::shared_ptr<GameObject> object_ptr, std::string name);
    std::shared_ptr<GameObject> GameObjectManager::GetGameObject(std::string name);
    static GameObjectManager* getInstance();
    void DrawCommnad(VkCommandBuffer& commandBuffer, uint32_t idx);
    void InitializePhysics();
    void UpdatePhysicTransforms();
    void Cleanup();
};

#endif
