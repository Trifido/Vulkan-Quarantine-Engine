#include "EditorObjectManager.h"
#include <iostream>

EditorObjectManager* EditorObjectManager::instance = nullptr;

void EditorObjectManager::AddEditorObject(std::shared_ptr<EditorObject> object_ptr, std::string name)
{
    this->_objects[name] = object_ptr;
}

EditorObjectManager* EditorObjectManager::getInstance()
{
    if (instance == NULL)
        instance = new EditorObjectManager();
    else
        std::cout << "Getting existing instance of EditorObjectManager" << std::endl;

    return instance;
}

void EditorObjectManager::DrawCommnad(VkCommandBuffer& commandBuffer, uint32_t idx)
{
    for (auto model : this->_objects)
    {
        model.second->Draw(commandBuffer, idx); 
    }
}

void EditorObjectManager::Cleanup()
{
    for (auto model : this->_objects)
    {
        model.second->Clean();
    }
}

std::shared_ptr<EditorObject> EditorObjectManager::GetObject(std::string name)
{
    auto it = this->_objects.find(name);

    if (it != this->_objects.end())
        return it->second;

    return nullptr;
}
