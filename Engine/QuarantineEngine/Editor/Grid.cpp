#include "Grid.h"
#include <ShaderManager.h>
#include <GraphicsPipelineModule.h>

Grid::Grid()
{
    this->gridMesh = std::make_unique<GameObject>(GameObject(PRIMITIVE_TYPE::GRID_TYPE));

    ShaderManager* shaderManager = ShaderManager::getInstance();
    this->shader_grid_ptr = std::make_shared<ShaderModule>(ShaderModule("../../resources/shaders/Grid/grid_vert.spv", "../../resources/shaders/Grid/grid_frag.spv"));
    shaderManager->AddShader("shader_grid", shader_grid_ptr);

    this->material_grid_ptr = std::make_shared<Material>(Material(this->shader_grid_ptr));
    this->material_grid_ptr->InitializeMaterialDataUBO();
                                                                        //CAMBIAR RENDER LAYER!!!

    //TextureManager* textureManager = TextureManager::getInstance();
    //this->material_grid_ptr->AddNullTexture(textureManager->GetTexture("NULL"));

    MaterialManager* instanceMaterialManager = MaterialManager::getInstance();
    std::string nameGrid = "editor:grid";
    instanceMaterialManager->AddMaterial(nameGrid, this->material_grid_ptr);
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
