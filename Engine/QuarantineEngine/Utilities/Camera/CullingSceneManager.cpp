#include "CullingSceneManager.h"
#include <MaterialManager.h>
#include <filesystem>

void CullingSceneManager::AddCameraFrustum(std::shared_ptr<FrustumComponent> frustum)
{
    this->cameraFrustum = frustum;
}

void CullingSceneManager::InitializeCullingSceneResources()
{
    this->aabb_objects.reserve(32);

    ShaderManager* shaderManager = ShaderManager::getInstance();

    this->shader_aabb_ptr = shaderManager->GetShader("shader_aabb_debug");

    MaterialManager* matManager = MaterialManager::getInstance();
    std::string nameDebugAABB = "editorDebugAABB";

    if (!matManager->Exists(nameDebugAABB))
    {
        this->material_aabb_ptr = std::make_shared<Material>(Material(nameDebugAABB, this->shader_aabb_ptr));
        this->material_aabb_ptr->layer = (unsigned int)RenderLayer::EDITOR;
        this->material_aabb_ptr->InitializeMaterialDataUBO();
        matManager->AddMaterial(this->material_aabb_ptr);
    }
    else
    {
        this->material_aabb_ptr = matManager->GetMaterial(nameDebugAABB);
        this->material_aabb_ptr->InitializeMaterialDataUBO();
    }
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
    if (this->DebugMode)
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
