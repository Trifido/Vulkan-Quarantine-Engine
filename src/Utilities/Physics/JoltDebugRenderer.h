#pragma once

#ifndef JOLT_DEBUG_RENDERER_H
#define JOLT_DEBUG_RENDERER_H

#include <DeviceModule.h>
#include <MaterialManager.h>
#include <Vertex.h>
#include <glm/glm.hpp>
#include <vector>
#include <vulkan/vulkan.hpp>

// Jolt
#include <Jolt/Jolt.h>
#include <Jolt/Renderer/DebugRenderer.h>

class JoltDebugRenderer : public JPH::DebugRenderer
{
private:
    static DeviceModule* deviceModule_ptr;
    VkBuffer lineVertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory lineVertexMemory = VK_NULL_HANDLE;
    std::vector<DebugVertex> lineVertices;

    glm::vec4 debugColor = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);

    std::shared_ptr<ShaderModule> shader_debug_ptr = nullptr;
    std::shared_ptr<QEMaterial> material_debug_ptr = nullptr;

    bool m_enabled = false;

private:
    void createVertexBuffer();
    void updateVertexBuffer();

    static inline glm::vec4 toRGBA(JPH::ColorArg col);
    static inline glm::vec4 toVec4(JPH::RVec3Arg p);
public:
    JoltDebugRenderer();

    void DrawLine(JPH::RVec3Arg inFrom, JPH::RVec3Arg inTo, JPH::ColorArg inColor) override;

    void DrawLine(JPH::RVec3Arg inFrom, JPH::ColorArg inColorFrom, JPH::RVec3Arg inTo, JPH::ColorArg inColorTo);

    void DrawTriangle(JPH::RVec3Arg v1, JPH::ColorArg c1, JPH::RVec3Arg v2, JPH::ColorArg c2, JPH::RVec3Arg v3, JPH::ColorArg c3);

    void DrawTriangle(JPH::RVec3Arg, JPH::RVec3Arg, JPH::RVec3Arg, JPH::ColorArg, JPH::DebugRenderer::ECastShadow) override {}

    void DrawText3D(JPH::RVec3Arg, const JPH::string_view&, JPH::ColorArg, float) override {}

    void DrawGeometry(JPH::RMat44Arg, const JPH::AABox&, float, JPH::ColorArg, const JPH::DebugRenderer::GeometryRef&, JPH::DebugRenderer::ECullMode, JPH::DebugRenderer::ECastShadow, JPH::DebugRenderer::EDrawMode) override {}

    JPH::DebugRenderer::Batch CreateTriangleBatch(const JPH::DebugRenderer::Vertex* inVertices,
        int inVertexCount,
        const JPH::uint32* inIndices,
        int inIndexCount) override;

    JPH::DebugRenderer::Batch CreateTriangleBatch(const JPH::DebugRenderer::Triangle* inTriangles,
        int inTriangleCount) override;

    void SetEnabled(bool b) { m_enabled = b; }
    bool IsEnabled() const { return m_enabled; }

    void DrawDebug(VkCommandBuffer& commandBuffer, uint32_t idx);
    void InitializeDebugResources();

    void UpdateBuffers();
    void clear();
    void cleanup();

    JPH::DebugRenderer& JoltIf() { return *this; }
};

#endif // !JOLT_DEBUG_RENDERER_H
