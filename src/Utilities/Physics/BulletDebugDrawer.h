#pragma once

#ifndef BULLET_DEBUG_DRAWER_H
#define BULLET_DEBUG_DRAWER_H

#include <DeviceModule.h>
#include <MaterialManager.h>
#include <Vertex.h>
#include <btBulletDynamicsCommon.h>
#include <glm/glm.hpp>
#include <vector>
#include <vulkan/vulkan.hpp>

class BulletDebugDrawer : public btIDebugDraw
{
private:
    static DeviceModule* deviceModule_ptr;
    VkBuffer lineVertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory lineVertexMemory = VK_NULL_HANDLE;
    std::vector<DebugVertex> lineVertices;
    int debugMode = DBG_DrawWireframe;
    glm::vec4 debugColor = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);

    std::shared_ptr<ShaderModule> shader_debug_ptr = nullptr;
    std::shared_ptr<Material> material_debug_ptr = nullptr;

public:
    bool DebugMode = false;

private:
    void createVertexBuffer();
    void updateVertexBuffer();

public:
    BulletDebugDrawer();
    void DrawDebug(VkCommandBuffer& commandBuffer, uint32_t idx);
    void drawLine(const btVector3& from, const btVector3& to, const btVector3& color) override;

    void drawContactPoint(const btVector3& PointOnB, const btVector3& normalOnB, btScalar distance, int lifeTime, const btVector3& color) override {}
    void draw3dText(const btVector3& location, const char* textString) override {}
    void reportErrorWarning(const char*) override {}
    void setDebugMode(int mode) override { debugMode = mode; }
    int getDebugMode() const override { return debugMode; }
    void InitializeDebugResources();

    void UpdateBuffers();
    void clear();
    void cleanup();
};

#endif // !1
