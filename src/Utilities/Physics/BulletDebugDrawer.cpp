#include "BulletDebugDrawer.h"
#include <BufferManageModule.h>

DeviceModule* BulletDebugDrawer::deviceModule_ptr;

BulletDebugDrawer::BulletDebugDrawer() : btIDebugDraw()
{
    deviceModule_ptr = DeviceModule::getInstance();
}

void BulletDebugDrawer::drawLine(const btVector3& from, const btVector3& to, const btVector3& color)
{
    glm::vec4 positionFrom = { from.x(), from.y(), from.z(), 1.0f };
    glm::vec4 positionTo = { to.x(), to.y(), to.z(), 1.0f };
    glm::vec4 colorVec = { color.x(), color.y(), color.z(), 1.0f };

    lineVertices.push_back(DebugVertex(positionFrom, debugColor));
    lineVertices.push_back(DebugVertex(positionTo, debugColor));
}

void BulletDebugDrawer::UpdateBuffers()
{
    if (lineVertices.empty())
        return;

    if (lineVertexMemory != VK_NULL_HANDLE)
    {
        this->updateVertexBuffer();
    }
    else
    {
        this->createVertexBuffer();
    }
}

void BulletDebugDrawer::clear()
{
    lineVertices.clear();
}

void BulletDebugDrawer::cleanup()
{
    if (lineVertexMemory != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(deviceModule_ptr->device, lineVertexBuffer, nullptr);
        vkFreeMemory(deviceModule_ptr->device, lineVertexMemory, nullptr);
        lineVertexBuffer = VK_NULL_HANDLE;
        lineVertexMemory = VK_NULL_HANDLE;
    }
}

void BulletDebugDrawer::createVertexBuffer()
{
    if (lineVertices.empty())
        return;

    VkDeviceSize bufferSize = sizeof(DebugVertex) * lineVertices.size();
    BufferManageModule::createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, lineVertexBuffer, lineVertexMemory, *deviceModule_ptr);

    this->updateVertexBuffer();
}

void BulletDebugDrawer::updateVertexBuffer()
{
    if (lineVertices.empty())
        return;

    VkDeviceSize bufferSize = sizeof(DebugVertex) * lineVertices.size();

    // 1. Crear staging buffer (host visible + coherent)
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingMemory;
    BufferManageModule::createBuffer(
        bufferSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer,
        stagingMemory,
        *deviceModule_ptr
    );

    // 2. Mapear staging y copiar los datos
    void* data;
    vkMapMemory(deviceModule_ptr->device, stagingMemory, 0, bufferSize, 0, &data);
    memcpy(data, lineVertices.data(), static_cast<size_t>(bufferSize));
    vkUnmapMemory(deviceModule_ptr->device, stagingMemory);

    // 3. Copiar desde staging al buffer en GPU (DEVICE_LOCAL)
    BufferManageModule::copyBuffer(
        stagingBuffer,
        lineVertexBuffer,
        bufferSize,
        *deviceModule_ptr
    );

    // 4. Limpiar staging buffer
    vkDestroyBuffer(deviceModule_ptr->device, stagingBuffer, nullptr);
    vkFreeMemory(deviceModule_ptr->device, stagingMemory, nullptr);
}

void BulletDebugDrawer::InitializeDebugResources()
{
    ShaderManager* shaderManager = ShaderManager::getInstance();

    this->shader_debug_ptr = shaderManager->GetShader("shader_debug_lines");

    MaterialManager* matManager = MaterialManager::getInstance();
    std::string nameDebugAABB = "editorDebugCollider";

    if (!matManager->Exists(nameDebugAABB))
    {
        this->material_debug_ptr = std::make_shared<Material>(Material(nameDebugAABB, this->shader_debug_ptr));
        this->material_debug_ptr->layer = (unsigned int)RenderLayer::EDITOR;
        this->material_debug_ptr->InitializeMaterialDataUBO();
        matManager->AddMaterial(this->material_debug_ptr);
    }
    else
    {
        this->material_debug_ptr = matManager->GetMaterial(nameDebugAABB);
        this->material_debug_ptr->InitializeMaterialDataUBO();
    }

    this->createVertexBuffer();
}

void BulletDebugDrawer::DrawDebug(VkCommandBuffer& commandBuffer, uint32_t idx)
{
    if (this->DebugMode && !this->lineVertices.empty())
    {
        auto pipelineModule = this->shader_debug_ptr->PipelineModule;
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineModule->pipeline);

        vkCmdSetDepthTestEnable(commandBuffer, false);
        vkCmdSetCullMode(commandBuffer, false);

        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineModule->pipelineLayout, 0, 1, this->material_debug_ptr->descriptor->getDescriptorSet(idx), 0, nullptr);

        VkDeviceSize offset = 0;
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &this->lineVertexBuffer, &offset);

        vkCmdDraw(commandBuffer, static_cast<uint32_t>(lineVertices.size()), 1, 0, 0);
    }
}
