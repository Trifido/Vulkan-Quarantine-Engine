#include "Grid.h"
#include <ShaderManager.h>
#include <GraphicsPipelineModule.h>
#include <filesystem>
#include <QEMeshRenderer.h>

Grid::Grid()
{
    std::shared_ptr<QEGeometryComponent> geometryComponent =
        std::make_shared<QEGeometryComponent>(std::make_unique<GridGenerator>());

    this->gridMesh = std::make_unique<QEGameObject>();
    this->gridMesh->AddComponent<QEGeometryComponent>(geometryComponent);

    EnsureResources();
}

void Grid::EnsureResources()
{
    if (!this->gridMesh)
        return;

    ShaderManager* shaderManager = ShaderManager::getInstance();
    MaterialManager* matManager = MaterialManager::getInstance();

    this->shader_grid_ptr = shaderManager->GetShader("shader_grid");
    if (!this->shader_grid_ptr)
        return;

    std::string nameGrid = "editorGrid";

    if (!matManager->Exists(nameGrid))
    {
        this->material_grid_ptr = std::make_shared<QEMaterial>(nameGrid, this->shader_grid_ptr);
        this->material_grid_ptr->renderQueue = static_cast<unsigned int>(RenderQueue::Editor);
        this->material_grid_ptr->InitializeMaterialData();
        matManager->AddMaterial(this->material_grid_ptr);
        matManager->MarkMaterialPersistent(nameGrid);
    }
    else
    {
        this->material_grid_ptr = matManager->GetMaterial(nameGrid);
        if (!this->material_grid_ptr)
            return;

        this->material_grid_ptr->shader = this->shader_grid_ptr;

        if (!this->material_grid_ptr->descriptor ||
            !this->material_grid_ptr->shader ||
            !this->material_grid_ptr->shader->PipelineModule ||
            this->material_grid_ptr->descriptor->descriptorSets.empty())
        {
            this->material_grid_ptr->InitializeMaterialData();
        }
    }

    this->gridMesh->SetMaterial(this->material_grid_ptr);
}

void Grid::Draw(VkCommandBuffer& commandBuffer, uint32_t idx)
{
    if (!this->gridMesh)
        return;

    auto mat = this->gridMesh->GetMaterial();
    if (!mat)
        return;

    if (!mat->shader)
        return;

    auto pipelineModule = mat->shader->PipelineModule;
    if (!pipelineModule)
        return;

    if (!mat->descriptor)
        return;

    if (idx >= mat->descriptor->descriptorSets.size())
        return;

    VkDescriptorSet set = mat->descriptor->descriptorSets[idx];
    if (set == VK_NULL_HANDLE)
        return;

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineModule->pipeline);

    vkCmdSetDepthTestEnable(commandBuffer, VK_TRUE);
    vkCmdSetDepthWriteEnable(commandBuffer, VK_FALSE);
    vkCmdSetFrontFace(commandBuffer, VK_FRONT_FACE_CLOCKWISE);
    vkCmdSetCullMode(commandBuffer, VK_CULL_MODE_NONE);

    vkCmdBindDescriptorSets(
        commandBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        pipelineModule->pipelineLayout,
        0,
        1,
        &set,
        0,
        nullptr
    );

    vkCmdDraw(commandBuffer, 6, 1, 0, 0);
}

void Grid::Clean()
{
    this->gridMesh->QEDestroy();
}
