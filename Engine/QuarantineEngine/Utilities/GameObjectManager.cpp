#include "GameObjectManager.h"

#include <iostream>

GameObjectManager* GameObjectManager::instance = nullptr;

GameObjectManager::GameObjectManager()
{
    this->renderLayers = RenderLayerModule::getInstance();
}

void GameObjectManager::AddGameObject(std::shared_ptr<GameObject> object_ptr, std::string name)
{
    if (object_ptr->IsValid())
    {
        this->_objects[object_ptr->material->layer][name] = object_ptr;

        if (object_ptr->physicBody != nullptr)
        {
            this->_physicObjects[name] = object_ptr;
        }
    }
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
    for (unsigned int idl = 0; idl < this->renderLayers->GetCount(); idl++)
    {
        for (auto model : this->_objects[this->renderLayers->GetLayer(idl)])
        {
            model.second->drawCommand(commandBuffer, idx);
        }
    }
}

void GameObjectManager::InitializePhysics()
{
    for (auto model : this->_physicObjects)
    {
        model.second->InitializePhysics();
    }
}

void GameObjectManager::UpdatePhysicTransforms()
{
    for (auto model : this->_physicObjects)
    {
        model.second->UpdatePhysicTransform();
    }
}

void GameObjectManager::Cleanup()
{
    for (unsigned int idl = 0; idl < this->renderLayers->GetCount(); idl++)
    {
        for (auto model : this->_objects[this->renderLayers->GetLayer(idl)])
        {
            model.second->cleanup();
        }
    }
}

std::shared_ptr<GameObject> GameObjectManager::GetGameObject(std::string name)
{
    for (unsigned int idl = 0; idl < this->renderLayers->GetCount(); idl++)
    {
        unsigned int id = this->renderLayers->GetLayer(idl);
        auto it = this->_objects[id].find(name);

        if (it != this->_objects[id].end())
            return it->second;
    }

    return nullptr;
}
