#include "QEMeshRenderer.h"
#include "QEGameObject.h"

QEMeshRenderer::QEMeshRenderer()
{
    this->deviceModule = DeviceModule::getInstance();
}

void QEMeshRenderer::QEStart()
{
    vkCmdDrawMeshTasksEXT = (PFN_vkCmdDrawMeshTasksEXT)vkGetDeviceProcAddr(this->deviceModule->device, "vkCmdDrawMeshTasksEXT");
    animationComponent = this->Owner->GetComponent<AnimationComponent>();
    geometryComponent = this->Owner->GetComponent<QEGeometryComponent>();
    materialComponent = this->Owner->GetComponent<Material>();
    transformComponent = this->Owner->GetComponent<Transform>();

    if (this->IsMeshShaderPipeline)
    {
        materialComponent->descriptor->SetMeshletBuffers(geometryComponent->meshlets_ptr[0]);
    }
}

void QEMeshRenderer::QEUpdate()
{
}

void QEMeshRenderer::QERelease()
{
}

void QEMeshRenderer::SetDrawCommand(VkCommandBuffer& commandBuffer, uint32_t idx)
{
    auto animator_ptr = this->animationComponent->animator;
    auto pipelineModule = this->materialComponent->shader->PipelineModule;
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineModule->pipeline);

    vkCmdSetDepthTestEnable(commandBuffer, true);
    vkCmdSetDepthWriteEnable(commandBuffer, true);


    vkCmdSetCullMode(commandBuffer, true);
    vkCmdSetFrontFace(commandBuffer, VK_FRONT_FACE_CLOCKWISE);

    for (int i = 0; i < this->geometryComponent->indexBuffer.size(); i++)
    {
        if (!this->IsMeshShaderPipeline)
        {
            VkDeviceSize offsets[] = { 0 };

            if (animator_ptr != nullptr)
            {
                vkCmdBindVertexBuffers(commandBuffer, 0, 1, &animator_ptr->GetComputeNode(std::to_string(i))->computeDescriptor->ssboData[2]->uniformBuffers.at(idx), offsets);
            }
            else
            {
                VkBuffer vertexBuffers[] = { this->geometryComponent->vertexBuffer[i] };
                vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
            }
            vkCmdBindIndexBuffer(commandBuffer, this->geometryComponent->indexBuffer[i], 0, VK_INDEX_TYPE_UINT32);
        }

        this->materialComponent->BindDescriptors(commandBuffer, idx);

        vkCmdPushConstants(commandBuffer, pipelineModule->pipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(PushConstantStruct), &this->transformComponent->GetModel());

        if (this->IsMeshShaderPipeline)
        {
            this->vkCmdDrawMeshTasksEXT(commandBuffer, 32, 1, 1);
        }
        else
        {
            auto indicesCount = this->geometryComponent->GetIndicesCount(i);
            vkCmdDrawIndexed(commandBuffer, indicesCount, 1, 0, 0, 0);
        }
    }
}

void QEMeshRenderer::SetDrawShadowCommand(VkCommandBuffer& commandBuffer, uint32_t idx, VkPipelineLayout pipelineLayout)
{
    auto animator_ptr = this->animationComponent->animator;
    for (int i = 0; i < geometryComponent->indexBuffer.size(); i++)
    {
        if (!this->IsMeshShaderPipeline)
        {
            VkDeviceSize offsets[] = { 0 };

            if (animator_ptr != nullptr)
            {
                vkCmdBindVertexBuffers(commandBuffer, 0, 1, &animator_ptr->GetComputeNode(std::to_string(i))->computeDescriptor->ssboData[2]->uniformBuffers.at(idx), offsets);
            }
            else
            {
                VkBuffer vertexBuffers[] = { geometryComponent->vertexBuffer[i] };
                vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
            }
            vkCmdBindIndexBuffer(commandBuffer, geometryComponent->indexBuffer[i], 0, VK_INDEX_TYPE_UINT32);
        }

        if (this->IsMeshShaderPipeline)
        {
            this->vkCmdDrawMeshTasksEXT(commandBuffer, 32, 1, 1);
        }
        else
        {
            auto indicesCount = geometryComponent->GetIndicesCount(i);
            vkCmdDrawIndexed(commandBuffer, indicesCount, 1, 0, 0, 0);
        }
    }
}
