#pragma once

#ifndef JOLT_DEBUG_RENDERER_H
#define JOLT_DEBUG_RENDERER_H

#include <Vertex.h>
#include <DebugSystem/QEDebugSystem.h>

// Jolt
#include <Jolt/Jolt.h>
#include <Jolt/Renderer/DebugRendererSimple.h>

class JoltDebugRenderer : public JPH::DebugRendererSimple
{
private:
    QEDebugSystem* debugSystem = nullptr;

    glm::vec4 debugColor = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);

private:
    static inline glm::vec4 toRGBA(JPH::ColorArg col);
    static inline glm::vec4 toVec4(JPH::RVec3Arg p);
public:
    JoltDebugRenderer();

    void DrawLine(JPH::RVec3Arg inFrom, JPH::RVec3Arg inTo, JPH::ColorArg inColor) override;

    void DrawLine(JPH::RVec3Arg inFrom, JPH::ColorArg inColorFrom, JPH::RVec3Arg inTo, JPH::ColorArg inColorTo);

    void DrawTriangle(JPH::RVec3Arg v1, JPH::ColorArg c1, JPH::RVec3Arg v2, JPH::ColorArg c2, JPH::RVec3Arg v3, JPH::ColorArg c3);

    void DrawTriangle(JPH::RVec3Arg inV1,
        JPH::RVec3Arg inV2,
        JPH::RVec3Arg inV3,
        JPH::ColorArg inColor,
        ECastShadow inCastShadow) override;

    void DrawText3D(JPH::RVec3Arg, const JPH::string_view&, JPH::ColorArg, float) override {}

    JPH::DebugRenderer& JoltIf() { return *this; }
};

#endif // !JOLT_DEBUG_RENDERER_H
