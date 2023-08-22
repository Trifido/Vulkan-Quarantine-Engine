#pragma once

#ifndef EDITOR_OBJECT_MANAGER_H
#define EDITOR_OBJECT_MANAGER_H

#include <unordered_map>
#include <memory>
#include "EditorObject.h"
#include <vulkan/vulkan.hpp>

class EditorObjectManager
{
private:
    std::unordered_map<std::string, std::shared_ptr<EditorObject>> _objects;
public:
    static EditorObjectManager* instance;

public:
    void AddEditorObject(std::shared_ptr<EditorObject> object_ptr, std::string name);
    std::shared_ptr<EditorObject> EditorObjectManager::GetObject(std::string name);
    static EditorObjectManager* getInstance();
    static void ResetInstance();
    void DrawCommnad(VkCommandBuffer& commandBuffer, uint32_t idx);
    void Cleanup();
    void CleanLastResources();
};

#endif
