#include "QEMeshRenderer.h"
#include "QEGameObject.h"

QEMeshRenderer::QEMeshRenderer()
    : materialComponents(*(new std::vector<std::shared_ptr<QEMaterial>>()))
{
    this->deviceModule = DeviceModule::getInstance();
    IsMeshShaderPipeline = false;
}

void QEMeshRenderer::QEStart()
{
    vkCmdDrawMeshTasksEXT = (PFN_vkCmdDrawMeshTasksEXT)vkGetDeviceProcAddr(this->deviceModule->device, "vkCmdDrawMeshTasksEXT");
    QEGameComponent::QEStart();
}

void QEMeshRenderer::QEInit()
{
    animationComponent = this->Owner->GetComponent<QEAnimationComponent>();
    geometryComponent = this->Owner->GetComponent<QEGeometryComponent>();
    RefreshMaterials();
    transformComponent = this->Owner->GetComponent<QETransform>();

    if (this->IsMeshShaderPipeline)
    {
        if (!materialComponents.empty())
        {
            materialComponents[0]->descriptor->SetMeshletBuffers(geometryComponent->meshlets_ptr[0]);
        }
    }
    QEGameComponent::QEInit();
}

void QEMeshRenderer::QEUpdate()
{
}

void QEMeshRenderer::QEDestroy()
{
    QEGameComponent::QEDestroy();
}

void QEMeshRenderer::RefreshMaterials()
{
    if (!this->Owner)
        return;

    materialComponents = this->Owner->GetMaterials();
}

void QEMeshRenderer::SetDrawCommand(VkCommandBuffer& commandBuffer, uint32_t idx)
{
    if (this->geometryComponent == nullptr || this->materialComponents.empty())
        return;

    auto qeMesh = this->geometryComponent->GetMesh();
    if (!qeMesh)
        return;

    for (uint32_t i = 0; i < this->geometryComponent->indexBuffer.size(); i++)
    {
        SetDrawCommand(commandBuffer, idx, i);
    }
}

void QEMeshRenderer::SetDrawCommand(VkCommandBuffer& commandBuffer, uint32_t idx, uint32_t subMeshIndex)
{
    if (this->geometryComponent == nullptr || this->materialComponents.empty())
        return;

    auto animator_ptr = (this->animationComponent != nullptr) ? this->animationComponent->animator : nullptr;
    auto pipelineModule = this->materialComponents[0]->shader->PipelineModule;
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineModule->pipeline);

    auto qeMesh = this->geometryComponent->GetMesh();
    if (!qeMesh || subMeshIndex >= this->geometryComponent->indexBuffer.size())
        return;

    if (!this->IsMeshShaderPipeline)
    {
        VkDeviceSize offsets[] = { 0 };

        if (animator_ptr != nullptr)
        {
            auto computeNode = animator_ptr->GetComputeNode(std::to_string(subMeshIndex));
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, &computeNode->computeDescriptor->ssboData[2]->uniformBuffers.at(idx), offsets);
        }
        else
        {
            VkBuffer vertexBuffers[] = { this->geometryComponent->vertexBuffer[subMeshIndex] };
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
        }
        vkCmdBindIndexBuffer(commandBuffer, this->geometryComponent->indexBuffer[subMeshIndex], 0, VK_INDEX_TYPE_UINT32);
    }

    std::string materialID;
    if (subMeshIndex < qeMesh->MaterialRel.size())
    {
        materialID = qeMesh->MaterialRel[subMeshIndex];
    }

    auto material = this->Owner->GetMaterial(materialID);
    if (!material)
    {
        material = this->Owner->GetMaterial();
    }

    if (!material)
        return;

    const bool isBlended =
        material->materialData.AlphaMode == 2u ||
        material->renderQueue >= static_cast<unsigned int>(RenderQueue::Transparent);

    vkCmdSetDepthTestEnable(commandBuffer, true);
    vkCmdSetDepthWriteEnable(commandBuffer, isBlended ? VK_FALSE : VK_TRUE);
    vkCmdSetFrontFace(commandBuffer, VK_FRONT_FACE_CLOCKWISE);

    material->BindDescriptors(commandBuffer, idx);

    vkCmdPushConstants(commandBuffer, pipelineModule->pipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(PushConstantStruct), &this->transformComponent->GetWorldMatrix());

    if (this->IsMeshShaderPipeline)
    {
        this->vkCmdDrawMeshTasksEXT(commandBuffer, 32, 1, 1);
    }
    else
    {
        auto indicesCount = this->geometryComponent->GetIndicesCount(subMeshIndex);
        vkCmdDrawIndexed(commandBuffer, indicesCount, 1, 0, 0, 0);
    }
}

void QEMeshRenderer::SetDrawShadowCommand(VkCommandBuffer& commandBuffer, uint32_t idx, VkPipelineLayout pipelineLayout)
{
    if (this->geometryComponent == nullptr)
        return;

    auto qeMesh = this->geometryComponent->GetMesh();
    if (!qeMesh)
        return;

    for (uint32_t i = 0; i < geometryComponent->indexBuffer.size(); i++)
    {
        SetDrawShadowCommand(commandBuffer, idx, pipelineLayout, i);
    }
}

void QEMeshRenderer::SetDrawShadowCommand(VkCommandBuffer& commandBuffer, uint32_t idx, VkPipelineLayout pipelineLayout, uint32_t subMeshIndex)
{
    if (this->geometryComponent == nullptr)
        return;

    auto animator_ptr = (this->animationComponent != nullptr) ? this->animationComponent->animator : nullptr;
    auto qeMesh = this->geometryComponent->GetMesh();
    if (!qeMesh || subMeshIndex >= geometryComponent->indexBuffer.size())
        return;

    if (!this->IsMeshShaderPipeline)
    {
        VkDeviceSize offsets[] = { 0 };

        if (animator_ptr != nullptr)
        {
            auto computeNode = animator_ptr->GetComputeNode(std::to_string(subMeshIndex));
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, &computeNode->computeDescriptor->ssboData[2]->uniformBuffers.at(idx), offsets);
        }
        else
        {
            VkBuffer vertexBuffers[] = { geometryComponent->vertexBuffer[subMeshIndex] };
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
        }
        vkCmdBindIndexBuffer(commandBuffer, geometryComponent->indexBuffer[subMeshIndex], 0, VK_INDEX_TYPE_UINT32);
    }

    if (subMeshIndex < qeMesh->MaterialRel.size())
    {
        auto material = this->Owner->GetMaterial(qeMesh->MaterialRel[subMeshIndex]);
        if (material)
        {
            const bool disableShadowCulling =
                material->materialData.DoubleSided ||
                material->materialData.AlphaMode != 0u;

            vkCmdSetCullMode(
                commandBuffer,
                disableShadowCulling ? VK_CULL_MODE_NONE : VK_CULL_MODE_BACK_BIT);

            if (material->HasDescriptorBuffer())
            {
                vkCmdBindDescriptorSets(
                    commandBuffer,
                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                    pipelineLayout,
                    1,
                    1,
                    material->descriptor->getDescriptorSet(idx),
                    0,
                    nullptr);
            }
        }
    }

    if (this->IsMeshShaderPipeline)
    {
        this->vkCmdDrawMeshTasksEXT(commandBuffer, 32, 1, 1);
    }
    else
    {
        auto indicesCount = geometryComponent->GetIndicesCount(subMeshIndex);
        vkCmdDrawIndexed(commandBuffer, indicesCount, 1, 0, 0, 0);
    }
}
