#include "CullingSceneManager.h"
#include <MaterialManager.h>
#include <filesystem>

CullingSceneManager* CullingSceneManager::instance = nullptr;

CullingSceneManager* CullingSceneManager::getInstance()
{
    if (instance == NULL)
        instance = new CullingSceneManager();

    return instance;
}

void CullingSceneManager::ResetInstance()
{
    delete instance;
    instance = nullptr;
}

void CullingSceneManager::AddCameraFrustum(std::shared_ptr<FrustumComponent> frustum)
{
    this->cameraFrustum = frustum;
}

void CullingSceneManager::InitializeCullingSceneResources()
{
    this->aabb_objects.reserve(32);

    ShaderManager* shaderManager = ShaderManager::getInstance();

    auto absPath = std::filesystem::absolute("../../resources/shaders").generic_string();

    std::string substring = "/Engine";
    std::size_t ind = absPath.find(substring);

    if (ind != std::string::npos) {
        absPath.erase(ind, substring.length());
    }

    const std::string absolute_debugBB_vertex_shader_path = absPath + "/Debug/debugAABB_vert.spv";
    const std::string absolute_debugBB_frag_shader_path = absPath + "/Debug/debugAABB_frag.spv";

    GraphicsPipelineData gpData = {};
    gpData.HasVertexData = true;
    gpData.polygonMode = VK_POLYGON_MODE_LINE;
    gpData.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
    gpData.vertexBufferStride = sizeof(glm::vec4);
    gpData.lineWidth = 2.0f;

    this->shader_aabb_ptr = std::make_shared<ShaderModule>(ShaderModule(absolute_debugBB_vertex_shader_path, absolute_debugBB_frag_shader_path, gpData));
    shaderManager->AddShader("shader_aabb_debug", shader_aabb_ptr);

    this->material_aabb_ptr = std::make_shared<Material>(Material(this->shader_aabb_ptr));
    this->material_aabb_ptr->layer = (unsigned int)RenderLayer::EDITOR;
    this->material_aabb_ptr->InitializeMaterialDataUBO();

    MaterialManager* instanceMaterialManager = MaterialManager::getInstance();
    std::string nameDebugAABB = "editor:debug_AABB";
    instanceMaterialManager->AddMaterial(nameDebugAABB, this->material_aabb_ptr);
}

std::shared_ptr<AABBObject> CullingSceneManager::GenerateAABB(std::pair<glm::vec3, glm::vec3> aabbData, std::shared_ptr<Transform> transform_ptr)
{
    AABBObject aabb;
    aabb.min = aabbData.first;
    aabb.max = aabbData.second;
    aabb.Size = (aabbData.second + aabbData.first) * 0.5f;
    aabb.Center = (aabbData.first - aabbData.second) * 0.5f;
    aabb.AddTransform(transform_ptr);

    aabb.CreateBuffers();

    this->aabb_objects.push_back(std::make_shared<AABBObject>(aabb));

    return this->aabb_objects.back();
}

void CullingSceneManager::CleanUp()
{
    for (unsigned int i = 0; i < this->aabb_objects.size(); i++)
    {
        this->aabb_objects.at(i)->CleanResources();
        this->aabb_objects.at(i).reset();
    }

    this->aabb_objects.clear();
}

void CullingSceneManager::DrawDebug(VkCommandBuffer& commandBuffer, uint32_t idx)
{
    if (this->isDebugEnable)
    {
        auto pipelineModule = this->shader_aabb_ptr->PipelineModule;
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineModule->pipeline);

        vkCmdSetDepthTestEnable(commandBuffer, false);
        vkCmdSetCullMode(commandBuffer, false);

        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineModule->pipelineLayout, 0, 1, this->material_aabb_ptr->descriptor->getDescriptorSet(idx), 0, nullptr);

        for (unsigned int i = 0; i < this->aabb_objects.size(); i++)
        {
            VkDeviceSize offsets[] = { 0 };
            VkBuffer vertexBuffers[] = { this->aabb_objects.at(i)->vertexBuffer };
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
            vkCmdBindIndexBuffer(commandBuffer, this->aabb_objects.at(i)->indexBuffer, 0, VK_INDEX_TYPE_UINT32);

            VkShaderStageFlagBits stages = VK_SHADER_STAGE_ALL;
            vkCmdPushConstants(commandBuffer, pipelineModule->pipelineLayout, stages, 0, sizeof(PushConstantStruct), &this->aabb_objects.at(i)->GetTransform()->GetModel());

            vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(this->aabb_objects.at(i)->indices.size()), 1, 0, 0, 0);
        }
    }
}

void CullingSceneManager::UpdateCullingScene()
{
    if (this->cameraFrustum->IsComputeCullingActive())
    {
        for (unsigned int i = 0; i < this->aabb_objects.size(); i++)
        {
            aabb_objects.at(i)->isGameObjectVisible = this->cameraFrustum->isAABBInside(*this->aabb_objects.at(i));
        }
        this->cameraFrustum->ActivateComputeCulling(false);
    }
}
