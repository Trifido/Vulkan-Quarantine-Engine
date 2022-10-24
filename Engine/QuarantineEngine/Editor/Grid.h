#pragma once
#ifndef GRID_H
#define GRID_H

#include "EditorObject.h"
#include <memory>
#include <GameObject.h>

class Grid : public EditorObject
{
private:
    std::shared_ptr<ShaderModule> shader_grid_ptr = nullptr;
    std::shared_ptr<Material> material_grid_ptr = nullptr;

public:
    std::unique_ptr<GameObject> gridMesh = nullptr;
    Grid(std::shared_ptr<GraphicsPipelineModule> graphicsPipelineModule, VkRenderPass renderPass);
    void Draw(VkCommandBuffer& commandBuffer, uint32_t idx);
    void Clean();
};

#endif
