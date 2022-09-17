#include "GameObject.h"
#include <PrimitiveMesh.h>

GameObject::GameObject()
{
    this->InitializeComponents();
}

GameObject::GameObject(PRIMITIVE_TYPE type, std::shared_ptr<DescriptorModule> descriptor)
{
    mesh = std::make_shared<PrimitiveMesh>(PrimitiveMesh(type));
    this->InitializeComponents();
    this->descriptorModule = descriptor;
}

GameObject::GameObject(std::string meshPath, std::shared_ptr<DescriptorModule> descriptor)
{
    mesh = std::make_shared<Mesh>(Mesh(meshPath));
    this->InitializeComponents();
    this->descriptorModule = descriptor;
}

void GameObject::cleanup()
{
    mesh->cleanup();
}

void GameObject::drawCommand(VkCommandBuffer& commandBuffer, uint32_t idx)
{
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, material->pipeline);
    VkBuffer vertexBuffers[] = { mesh->vertexBuffer };
    //VkBuffer* indexBuffers = &mesh->indexBuffer;
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, mesh->indexBuffer, 0, VK_INDEX_TYPE_UINT32);

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, material->pipelineLayout, 0, 1, descriptorModule->getDescriptorSet(idx), 0, nullptr);
    //vkCmdDraw(commandBuffers[i], static_cast<uint32_t>(geometryModule.vertices.size()), 1, 0, 0);

    vkCmdPushConstants(commandBuffer, material->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(transform->ubo.mvp), &transform->ubo.mvp);

    vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(mesh->indices.size()), 1, 0, 0, 0);
    //vkCmdDraw(commandBuffers[i], 3, 1, 0, 0);
}

void GameObject::addMaterial(std::shared_ptr<Material> material_ptr)
{
    this->material = material_ptr;
}

void GameObject::InitializeComponents()
{
    deviceModule = DeviceModule::getInstance();
    queueModule = QueueModule::getInstance();
    transform = std::make_shared<Transform>();
    mesh->InitializeMesh();
}
