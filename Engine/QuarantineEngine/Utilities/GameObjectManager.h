#pragma once

#ifndef GAME_OBJECT_MANAGER_H
#define GAME_OBJECT_MANAGER_H

#include <unordered_map>
#include <memory>
#include "GameObject.h"

class GameObjectManager
{
private:
    std::unordered_map<std::string, std::shared_ptr<GameObject>> _objects;
public:
    static GameObjectManager* instance;

public:
    void AddGameObject(std::shared_ptr<GameObject> object_ptr, std::string name);
    std::shared_ptr<GameObject> GameObjectManager::GetGameObject(std::string name);
    static GameObjectManager* getInstance();
    void DrawCommnad(VkCommandBuffer& commandBuffer, uint32_t idx);
    void InitializePhysics();
    void UpdatePhysicTransforms();
    void Cleanup();
};

#endif
