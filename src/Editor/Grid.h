#pragma once
#ifndef GRID_H
#define GRID_H

#include "EditorObject.h"
#include <memory>
#include <QEGameObject.h>

class Grid : public EditorObject
{
private:
    std::shared_ptr<ShaderModule> shader_grid_ptr = nullptr;
    std::shared_ptr<QEMaterial> material_grid_ptr = nullptr;

public:
    std::unique_ptr<QEGameObject> gridMesh = nullptr;
    Grid();
    void Draw(VkCommandBuffer& commandBuffer, uint32_t idx);
    void Clean();
};

#endif
