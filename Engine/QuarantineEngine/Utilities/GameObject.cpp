#include "GameObject.h"

GameObject::GameObject()
{
    deviceModule = DeviceModule::getInstance();
    queueModule = QueueModule::getInstance();
}

GameObject::GameObject(std::string meshPath, std::string albedoPath, uint32_t numSwapChain, VkCommandPool& commandPool)
{
    deviceModule = DeviceModule::getInstance();
    queueModule = QueueModule::getInstance();

    material = std::make_shared<Material>();
    material->addAlbedo(albedoPath, commandPool);
    mesh = std::make_shared<Mesh>(Mesh(*deviceModule, commandPool, *queueModule));
    mesh->loadMesh(meshPath);
    transform = std::make_shared<Transform>();

    descriptorModule = std::make_shared<DescriptorModule>(DescriptorModule(*deviceModule));
    descriptorModule->createUniformBuffers(numSwapChain);
    descriptorModule->addPtrData(material->getAlbedo());
    descriptorModule->createDescriptorSetLayout();
    descriptorModule->createDescriptorPool(numSwapChain);
    descriptorModule->createDescriptorSets();
}

void GameObject::cleanup()
{
    material->cleanup();
    mesh->cleanup();
}

void GameObject::drawCommand(VkCommandBuffer& commandBuffer, VkPipelineLayout& pipelineLayout, VkPipeline& pipeline, unsigned int idx)
{
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    VkBuffer vertexBuffers[] = { mesh->vertexBuffer };
    VkBuffer* indexBuffers = &mesh->indexBuffer;
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, mesh->indexBuffer, 0, VK_INDEX_TYPE_UINT32);

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, descriptorModule->getDescriptorSet(idx), 0, nullptr);
    //vkCmdDraw(commandBuffers[i], static_cast<uint32_t>(geometryModule.vertices.size()), 1, 0, 0);

    vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(transform->ubo.mvp), &transform->ubo.mvp);

    vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(mesh->indices.size()), 1, 0, 0, 0);
    //vkCmdDraw(commandBuffers[i], 3, 1, 0, 0);
}
