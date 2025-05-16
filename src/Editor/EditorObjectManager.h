#pragma once

#ifndef EDITOR_OBJECT_MANAGER_H
#define EDITOR_OBJECT_MANAGER_H

#include <unordered_map>
#include <memory>
#include "EditorObject.h"
#include <vulkan/vulkan.hpp>
#include <QESingleton.h>

class EditorObjectManager : public QESingleton<EditorObjectManager>
{
private:
    friend class QESingleton<EditorObjectManager>; // Permitir acceso al constructor
    std::unordered_map<std::string, std::shared_ptr<EditorObject>> _objects;

public:
    void AddEditorObject(std::shared_ptr<EditorObject> object_ptr, std::string name);
    std::shared_ptr<EditorObject> GetObject(std::string name);
    void DrawCommnad(VkCommandBuffer& commandBuffer, uint32_t idx);
    void Cleanup();
    void CleanLastResources();
};

#endif
