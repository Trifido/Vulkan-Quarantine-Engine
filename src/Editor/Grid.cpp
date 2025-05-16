#include "Grid.h"
#include <ShaderManager.h>
#include <GraphicsPipelineModule.h>
#include <filesystem>

Grid::Grid()
{
    this->gridMesh = std::make_unique<GameObject>(GameObject(PRIMITIVE_TYPE::GRID_TYPE));

    ShaderManager* shaderManager = ShaderManager::getInstance();

    this->shader_grid_ptr = shaderManager->GetShader("shader_grid");

    MaterialManager* matManager = MaterialManager::getInstance();

    std::string nameGrid = "editorGrid";
    if (!matManager->Exists(nameGrid))
    {
        this->material_grid_ptr = std::make_shared<Material>(Material(nameGrid, this->shader_grid_ptr));
        this->material_grid_ptr->layer = (unsigned int)RenderLayer::EDITOR;
        this->material_grid_ptr->InitializeMaterialDataUBO();
        matManager->AddMaterial(this->material_grid_ptr);
    }
    else
    {
        this->material_grid_ptr = matManager->GetMaterial(nameGrid);
        this->material_grid_ptr->InitializeMaterialDataUBO();
    }
    this->gridMesh->AddMaterial(this->material_grid_ptr);
}

void Grid::Draw(VkCommandBuffer& commandBuffer, uint32_t idx)
{
    auto pipelineModule = this->gridMesh->_Material->shader->PipelineModule;
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineModule->pipeline);

    vkCmdSetDepthTestEnable(commandBuffer, true);
    vkCmdSetDepthWriteEnable(commandBuffer, true);
    vkCmdSetFrontFace(commandBuffer, VK_FRONT_FACE_CLOCKWISE);
    vkCmdSetCullMode(commandBuffer, false);

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineModule->pipelineLayout, 0, 1, this->gridMesh->_Material->descriptor->getDescriptorSet(idx), 0, nullptr);

    vkCmdDraw(commandBuffer, 6, 1, 0, 0);
}

void Grid::Clean()
{
    this->gridMesh->Cleanup();
}
