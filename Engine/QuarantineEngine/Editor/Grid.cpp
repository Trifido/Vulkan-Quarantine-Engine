#include "Grid.h"
#include <ShaderManager.h>

Grid::Grid(std::shared_ptr<GraphicsPipelineModule> graphicsPipelineModule, VkRenderPass renderPass)
{
    this->gridMesh = std::make_unique<GameObject>(GameObject(PRIMITIVE_TYPE::GRID_TYPE));

    ShaderManager* shaderManager = ShaderManager::getInstance();
    this->shader_grid_ptr = std::make_shared<ShaderModule>(ShaderModule("../../resources/shaders/Grid/grid_vert.spv", "../../resources/shaders/Grid/grid_frag.spv"));
    shader_grid_ptr->createShaderBindings();
    shaderManager->AddShader("shader_grid", shader_grid_ptr);

    this->material_grid_ptr = std::make_shared<Material>(Material(this->shader_grid_ptr, renderPass));

    TextureManager* textureManager = TextureManager::getInstance();
    this->material_grid_ptr->AddNullTexture(textureManager->GetTexture("NULL"));
    this->material_grid_ptr->AddPipeline(graphicsPipelineModule);

    MaterialManager* instanceMaterialManager = MaterialManager::getInstance();
    instanceMaterialManager->AddMaterial("editor:grid", this->material_grid_ptr);
    this->gridMesh->addMaterial(this->material_grid_ptr);
}

void Grid::Draw(VkCommandBuffer& commandBuffer, uint32_t idx)
{
    this->gridMesh->drawCommand(commandBuffer, idx);
}

void Grid::Clean()
{
    this->gridMesh->cleanup();
}
