#include "JoltDebugRenderer.h"
#include "Material.h"
#include <BufferManageModule.h>
#include "PhysicsModule.h"

DeviceModule* JoltDebugRenderer::deviceModule_ptr;

JoltDebugRenderer::JoltDebugRenderer()
{
    deviceModule_ptr = DeviceModule::getInstance();
}

void JoltDebugRenderer::DrawLine(JPH::RVec3Arg a, JPH::RVec3Arg b, JPH::ColorArg col)
{
    glm::vec4 c = toRGBA(col);
    lineVertices.emplace_back(toVec4(a), c);
    lineVertices.emplace_back(toVec4(b), c);
}

void JoltDebugRenderer::DrawLine(JPH::RVec3Arg a, JPH::ColorArg ca,
    JPH::RVec3Arg b, JPH::ColorArg cb)
{
    lineVertices.emplace_back(toVec4(a), toRGBA(ca));
    lineVertices.emplace_back(toVec4(b), toRGBA(cb));
}

void JoltDebugRenderer::DrawTriangle(JPH::RVec3Arg v1, JPH::ColorArg c1,
    JPH::RVec3Arg v2, JPH::ColorArg c2,
    JPH::RVec3Arg v3, JPH::ColorArg c3)
{
    // Wireframe = 3 líneas
    DrawLine(v1, c1, v2, c2);
    DrawLine(v2, c2, v3, c3);
    DrawLine(v3, c3, v1, c1);
}

JPH::DebugRenderer::Batch JoltDebugRenderer::CreateTriangleBatch(const JPH::DebugRenderer::Vertex* inVertices, int inVertexCount, const JPH::uint32* inIndices, int inIndexCount)
{
    return JPH::DebugRenderer::Batch();
}

JPH::DebugRenderer::Batch JoltDebugRenderer::CreateTriangleBatch(const JPH::DebugRenderer::Triangle* inTriangles, int inTriangleCount)
{
    return JPH::DebugRenderer::Batch();
}

void JoltDebugRenderer::UpdateBuffers()
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

void JoltDebugRenderer::clear()
{
    lineVertices.clear();
}

void JoltDebugRenderer::cleanup()
{
    if (lineVertexMemory != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(deviceModule_ptr->device, lineVertexBuffer, nullptr);
        vkFreeMemory(deviceModule_ptr->device, lineVertexMemory, nullptr);
        lineVertexBuffer = VK_NULL_HANDLE;
        lineVertexMemory = VK_NULL_HANDLE;
    }
}

void JoltDebugRenderer::createVertexBuffer()
{
    if (lineVertices.empty())
        return;

    VkDeviceSize bufferSize = sizeof(DebugVertex) * lineVertices.size();
    BufferManageModule::createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, lineVertexBuffer, lineVertexMemory, *deviceModule_ptr);

    this->updateVertexBuffer();
}

void JoltDebugRenderer::updateVertexBuffer()
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

void JoltDebugRenderer::DrawDebug(VkCommandBuffer& commandBuffer, uint32_t idx)
{
    if (this->m_enabled && !this->lineVertices.empty())
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

inline glm::vec4 JoltDebugRenderer::toRGBA(JPH::ColorArg col)
{
    const float inv = 1.0f / 255.0f;
    const uint32_t u = col.GetUInt32(); // 0xRRGGBBAA en Jolt
    const float r = ((u >> 24) & 0xFF) * inv;
    const float g = ((u >> 16) & 0xFF) * inv;
    const float b = ((u >> 8) & 0xFF) * inv;
    const float a = (u & 0xFF) * inv;
    return glm::vec4(r, g, b, a);
}

inline glm::vec4 JoltDebugRenderer::toVec4(JPH::RVec3Arg p)
{
    return glm::vec4((float)p.GetX(), (float)p.GetY(), (float)p.GetZ(), 1.0f);
}

void JoltDebugRenderer::InitializeDebugResources()
{
    ShaderManager* shaderManager = ShaderManager::getInstance();

    this->shader_debug_ptr = shaderManager->GetShader("shader_debug_lines");

    MaterialManager* matManager = MaterialManager::getInstance();
    std::string nameDebugAABB = "editorDebugCollider";

    if (!matManager->Exists(nameDebugAABB))
    {
        this->material_debug_ptr = std::make_shared<QEMaterial>(QEMaterial(nameDebugAABB, this->shader_debug_ptr));
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
