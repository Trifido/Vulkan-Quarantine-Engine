#include "QEDebugSystem.h"
#include <BufferManageModule.h>

QEDebugSystem::QEDebugSystem()
{
    deviceModule_ptr = DeviceModule::getInstance();
}

void QEDebugSystem::createVertexBuffer()
{
    if (lineVertices.empty())
        return;

    VkDeviceSize bufferSize = sizeof(DebugVertex) * lineVertices.size();
    BufferManageModule::createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, lineVertexBuffer, lineVertexMemory, *deviceModule_ptr);

    this->updateVertexBuffer();
}

void QEDebugSystem::updateVertexBuffer()
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

void QEDebugSystem::InitializeDebugGraphicResources()
{
    ShaderManager* shaderManager = ShaderManager::getInstance();

    this->shader_debug_ptr = shaderManager->GetShader("shader_debug_lines");

    MaterialManager* matManager = MaterialManager::getInstance();
    std::string nameDebugAABB = "editorDebugCollider";

    if (!matManager->Exists(nameDebugAABB))
    {
        this->material_debug_ptr = std::make_shared<QEMaterial>(nameDebugAABB, this->shader_debug_ptr);
        this->material_debug_ptr->layer = (unsigned int)RenderLayer::EDITOR;
        this->material_debug_ptr->InitializeMaterialData();
        matManager->AddMaterial(this->material_debug_ptr);
    }
    else
    {
        this->material_debug_ptr = matManager->GetMaterial(nameDebugAABB);
        this->material_debug_ptr->InitializeMaterialData();
    }

    this->createVertexBuffer();
}

void QEDebugSystem::AddLine(glm::vec3 orig, glm::vec3 end, glm::vec4 color)
{
    lineVertices.emplace_back(glm::vec4(orig, 1.0), color);
    lineVertices.emplace_back(glm::vec4(end, 1.0), color);
}

void QEDebugSystem::AddLine(glm::vec3 orig, glm::vec3 end, glm::vec4 origColor, glm::vec4 endColor)
{
    lineVertices.emplace_back(glm::vec4(orig, 1.0), origColor);
    lineVertices.emplace_back(glm::vec4(end, 1.0), endColor);
}

void QEDebugSystem::AddLine(glm::vec4 orig, glm::vec4 end, glm::vec4 color)
{
    lineVertices.emplace_back(orig, color);
    lineVertices.emplace_back(end, color);
}

void QEDebugSystem::AddLine(glm::vec4 orig, glm::vec4 end, glm::vec4 origColor, glm::vec4 endColor)
{
    lineVertices.emplace_back(orig, origColor);
    lineVertices.emplace_back(end, endColor);
}

void QEDebugSystem::AddPoint(glm::vec4 orig, float size, glm::vec4 color)
{
    glm::vec4 forward = orig + glm::vec4(0.0f, 0.0f, 1.0f, 0.0f) * size;
    glm::vec4 up = orig + glm::vec4(0.0f, 1.0f, 0.0f, 0.0f) * size;
    glm::vec4 right = orig + glm::vec4(1.0f, 0.0f, 0.0f, 0.0f) * size;

    AddLine(orig, forward, color);
    AddLine(orig, -forward, color);
    AddLine(orig, up, color);
    AddLine(orig, -up, color);
    AddLine(orig, right, color);
    AddLine(orig, -right, color);
}

void QEDebugSystem::DrawDebugLines(VkCommandBuffer& commandBuffer, uint32_t idx)
{
    if (enabled && !lineVertices.empty())
    {
        auto pipelineModule = this->shader_debug_ptr->PipelineModule;
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineModule->pipeline);

        vkCmdSetDepthTestEnable(commandBuffer, false);
        vkCmdSetCullMode(commandBuffer, false);

        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineModule->pipelineLayout, 0, 1, material_debug_ptr->descriptor->getDescriptorSet(idx), 0, nullptr);

        VkDeviceSize offset = 0;
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &lineVertexBuffer, &offset);

        vkCmdDraw(commandBuffer, static_cast<uint32_t>(lineVertices.size()), 1, 0, 0);
    }
}

void QEDebugSystem::UpdateGraphicBuffers()
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

void QEDebugSystem::ClearLines()
{
    lineVertices.clear();
}

void QEDebugSystem::Cleanup()
{
    if (lineVertexMemory != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(deviceModule_ptr->device, lineVertexBuffer, nullptr);
        vkFreeMemory(deviceModule_ptr->device, lineVertexMemory, nullptr);
        lineVertexBuffer = VK_NULL_HANDLE;
        lineVertexMemory = VK_NULL_HANDLE;
    }
}
