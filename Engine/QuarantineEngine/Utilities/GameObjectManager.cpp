#include "GameObjectManager.h"

#include <iostream>

GameObjectManager* GameObjectManager::instance = nullptr;

GameObjectManager::GameObjectManager()
{
    this->renderLayers = RenderLayerModule::getInstance();
}

std::string GameObjectManager::CheckName(std::string nameGameObject)
{
    std::unordered_map<std::string, std::shared_ptr<GameObject>>::const_iterator got;
    std::string newName = nameGameObject;
    unsigned int id = 0;

    for (unsigned int idl = 0; idl < this->renderLayers->GetCount(); idl++)
    {
        do
        {
            got = this->_objects[this->renderLayers->GetLayer(idl)].find(newName);

            if (got != this->_objects[this->renderLayers->GetLayer(idl)].end())
            {
                id++;
                newName = nameGameObject + "_" + std::to_string(id);
            }
        } while (got != this->_objects[this->renderLayers->GetLayer(idl)].end());
    }

    return newName;
}

void GameObjectManager::AddGameObject(std::shared_ptr<GameObject> object_ptr, std::string name)
{
    if (object_ptr->IsValid())
    {
        name = CheckName(name);

        if (object_ptr->material == nullptr)
        {
            if (!object_ptr->childs.empty())
            {
                unsigned int childLayer = object_ptr->childs[0]->material->layer;
                this->_objects[childLayer][name] = object_ptr;
            }
        }
        else
        {
            this->_objects[object_ptr->material->layer][name] = object_ptr;
        }

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

    return instance;
}

void GameObjectManager::ResetInstance()
{
	delete instance;
	instance = nullptr;
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

void GameObjectManager::CleanLastResources()
{
    this->_objects.clear();
    this->_physicObjects.clear();
    delete this->renderLayers;
    this->renderLayers = nullptr;
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
