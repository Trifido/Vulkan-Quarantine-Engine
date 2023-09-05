#include "Grid.h"
#include <ShaderManager.h>
#include <GraphicsPipelineModule.h>
#include <filesystem>

Grid::Grid()
{
    this->gridMesh = std::make_unique<GameObject>(GameObject(PRIMITIVE_TYPE::GRID_TYPE));

    ShaderManager* shaderManager = ShaderManager::getInstance();

    auto absPath = std::filesystem::absolute("../../resources/shaders").generic_string();

    std::string substring = "/Engine";
    std::size_t ind = absPath.find(substring);

    if (ind != std::string::npos) {
        absPath.erase(ind, substring.length());
    }

    const std::string absolute_grid_vertex_shader_path = absPath + "/Grid/grid_vert.spv";
    const std::string absolute_grid_frag_shader_path = absPath + "/Grid/grid_frag.spv";

    this->shader_grid_ptr = std::make_shared<ShaderModule>(ShaderModule(absolute_grid_vertex_shader_path, absolute_grid_frag_shader_path));
    shaderManager->AddShader("shader_grid", shader_grid_ptr);

    this->material_grid_ptr = std::make_shared<Material>(Material(this->shader_grid_ptr));
    this->material_grid_ptr->layer = (unsigned int)RenderLayer::EDITOR;
    this->material_grid_ptr->InitializeMaterialDataUBO();

    MaterialManager* instanceMaterialManager = MaterialManager::getInstance();
    std::string nameGrid = "editor:grid";
    instanceMaterialManager->AddMaterial(nameGrid, this->material_grid_ptr);
    this->gridMesh->addMaterial(this->material_grid_ptr);
}

void Grid::Draw(VkCommandBuffer& commandBuffer, uint32_t idx)
{
    auto pipelineModule = this->gridMesh->material->shader->PipelineModule;
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineModule->pipeline);

    vkCmdSetDepthTestEnable(commandBuffer, true);
    vkCmdSetCullMode(commandBuffer, false);

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineModule->pipelineLayout, 0, 1, this->gridMesh->material->descriptor->getDescriptorSet(idx), 0, nullptr);

    vkCmdDraw(commandBuffer, 6, 1, 0, 0);
}

void Grid::Clean()
{
    this->gridMesh->cleanup();
}
