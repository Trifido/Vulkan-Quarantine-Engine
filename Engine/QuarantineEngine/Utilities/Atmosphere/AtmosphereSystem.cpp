#include "AtmosphereSystem.h"
#include <ShaderManager.h>
#include <MaterialManager.h>
#include <filesystem>
#include <PrimitiveMesh.h>

AtmosphereSystem::AtmosphereSystem()
{
    this->device_ptr = DeviceModule::getInstance();
}

AtmosphereSystem::~AtmosphereSystem()
{
    this->device_ptr = nullptr;
}

void AtmosphereSystem::InitializeResources()
{
    auto absPath = std::filesystem::absolute("../../resources/shaders").generic_string();

    std::string substring = "/Engine";
    std::size_t ind = absPath.find(substring);

    if (ind != std::string::npos) {
        absPath.erase(ind, substring.length());
    }

    const std::string absolute_skybox_cubemap_vert_shader_path = absPath + "/Atmosphere/skybox_cubemap_vert.spv";
    const std::string absolute_skybox_cubemap_frag_shader_path = absPath + "/Atmosphere/skybox_cubemap_frag.spv";

    GraphicsPipelineData pipelineData = {};
    pipelineData.vertexBufferStride = sizeof(glm::vec4);

    auto shaderManager = ShaderManager::getInstance();
    this->skybox_cubemap_shader = std::make_shared<ShaderModule>(
        ShaderModule(absolute_skybox_cubemap_vert_shader_path, absolute_skybox_cubemap_frag_shader_path, pipelineData)
    );
    shaderManager->AddShader("skybox_cubemap", this->skybox_cubemap_shader);

    auto materialManager = MaterialManager::getInstance();

    if (this->skybox_cubemap_shader != nullptr)
    {
        materialManager->AddMaterial("skyboxCubemapMat", std::make_shared<Material>(Material(this->skybox_cubemap_shader)));
        materialManager->GetMaterial("skyboxCubemapMat")->layer = (int)RenderLayer::ENVIRONMENT;
    }

    this->_Mesh = std::make_shared<PrimitiveMesh>(PRIMITIVE_TYPE::CUBE_TYPE);
    this->_Mesh->InitializeMesh();
}

void AtmosphereSystem::DrawCommand(VkCommandBuffer& commandBuffer, uint32_t idx)
{
    if (this->_Mesh != nullptr)
    {
        auto pipelineModule = this->skybox_cubemap_shader->PipelineModule;
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineModule->pipeline);

        vkCmdSetDepthTestEnable(commandBuffer, true);
        vkCmdSetDepthWriteEnable(commandBuffer, true);

        vkCmdSetCullMode(commandBuffer, true);
        vkCmdSetFrontFace(commandBuffer, VK_FRONT_FACE_CLOCKWISE);

        VkDeviceSize offsets[] = { 0 };
        VkBuffer vertexBuffers[] = { this->_Mesh->vertexBuffer };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(commandBuffer, this->_Mesh->indexBuffer, 0, VK_INDEX_TYPE_UINT32);

        //this->_Material->BindDescriptors(commandBuffer, idx);

        vkCmdPushConstants(commandBuffer, pipelineModule->pipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(PushConstantStruct), &this->model);

        vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(this->_Mesh->indices.size()), 1, 0, 0, 0);
    }
}

void AtmosphereSystem::Cleanup()
{
    this->_Mesh->cleanup();
    this->_Mesh = nullptr;
}

void AtmosphereSystem::CleanLastResources()
{
    this->skybox_cubemap_shader.reset();
    this->skybox_cubemap_shader = nullptr;
}
