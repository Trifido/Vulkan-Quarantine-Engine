#include "GameObjectManager.h"

#include <iostream>

GameObjectManager* GameObjectManager::instance = nullptr;

void GameObjectManager::AddGameObject(std::shared_ptr<GameObject> object_ptr, std::string name)
{
    this->_objects[name] = object_ptr;
}

GameObjectManager* GameObjectManager::getInstance()
{
    if (instance == NULL)
        instance = new GameObjectManager();
    else
        std::cout << "Getting existing instance of EditorObjectManager" << std::endl;

    return instance;
}

void GameObjectManager::DrawCommnad(VkCommandBuffer& commandBuffer, uint32_t idx)
{
    for (auto model : this->_objects)
    {
        model.second->drawCommand(commandBuffer, idx);
    }
}

void GameObjectManager::InitializePhysics()
{
    for (auto model : this->_objects)
    {
        model.second->InitializePhysics();
    }
}

void GameObjectManager::UpdatePhysicTransforms()
{
    for (auto model : this->_objects)
    {
        model.second->UpdatePhysicTransform();
    }
}

void GameObjectManager::Cleanup()
{
    for (auto model : this->_objects)
    {
        model.second->cleanup();
    }
}

std::shared_ptr<GameObject> GameObjectManager::GetGameObject(std::string name)
{
    auto it = this->_objects.find(name);

    if (it != this->_objects.end())
        return it->second;

    return nullptr;
}
