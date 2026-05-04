#include "JoltDebugRenderer.h"

JoltDebugRenderer::JoltDebugRenderer()
{
    Initialize();
    debugSystem = QEDebugSystem::getInstance();
}

void JoltDebugRenderer::DrawLine(JPH::RVec3Arg a, JPH::RVec3Arg b, JPH::ColorArg col)
{
    debugSystem->AddLine(toVec4(a), toVec4(b), toRGBA(col));
}

void JoltDebugRenderer::DrawLine(JPH::RVec3Arg a, JPH::ColorArg ca,
    JPH::RVec3Arg b, JPH::ColorArg cb)
{
    debugSystem->AddLine(toVec4(a), toVec4(b), toRGBA(ca), toRGBA(cb));
}

void JoltDebugRenderer::DrawTriangle(JPH::RVec3Arg v1, JPH::ColorArg c1,
    JPH::RVec3Arg v2, JPH::ColorArg c2,
    JPH::RVec3Arg v3, JPH::ColorArg c3)
{
    DrawLine(v1, c1, v2, c2);
    DrawLine(v2, c2, v3, c3);
    DrawLine(v3, c3, v1, c1);
}

void JoltDebugRenderer::DrawTriangle(JPH::RVec3Arg inV1, JPH::RVec3Arg inV2, JPH::RVec3Arg inV3, JPH::ColorArg inColor, ECastShadow inCastShadow)
{
    debugSystem->AddLine(toVec4(inV1), toVec4(inV2), toRGBA(inColor));
    debugSystem->AddLine(toVec4(inV2), toVec4(inV3), toRGBA(inColor));
    debugSystem->AddLine(toVec4(inV3), toVec4(inV1), toRGBA(inColor));
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
