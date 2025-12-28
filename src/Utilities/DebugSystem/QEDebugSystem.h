#pragma once
#ifndef DEBUG_SYSTEM_H
#define DEBUG_SYSTEM_H

#include <QESingleton.h>
#include <DeviceModule.h>
#include <Vertex.h>
#include <Material.h>

class QEDebugSystem : public QESingleton<QEDebugSystem>
{
private:
    friend class QESingleton<QEDebugSystem>;

    DeviceModule* deviceModule_ptr = nullptr;
    VkBuffer lineVertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory lineVertexMemory = VK_NULL_HANDLE;
    std::vector<DebugVertex> lineVertices;

    glm::vec4 debugColor = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);

    std::shared_ptr<ShaderModule> shader_debug_ptr = nullptr;
    std::shared_ptr<QEMaterial> material_debug_ptr = nullptr;

    bool enabled = false;

private:
    void createVertexBuffer();
    void updateVertexBuffer();

public:
    QEDebugSystem();

    void SetEnabled(bool b) { enabled = b; }
    bool IsEnabled() const { return enabled; }

    void DrawDebugLines(VkCommandBuffer& commandBuffer, uint32_t idx);
    void InitializeDebugGraphicResources();

    void AddLine(glm::vec3 orig, glm::vec3 end, glm::vec4 color);
    void AddLine(glm::vec3 orig, glm::vec3 end, glm::vec4 origColor, glm::vec4 endColor);
    void AddLine(glm::vec4 orig, glm::vec4 end, glm::vec4 color);
    void AddLine(glm::vec4 orig, glm::vec4 end, glm::vec4 origColor, glm::vec4 endColor);
    void AddPoint(glm::vec4 orig, float size, glm::vec4 color);

    void UpdateGraphicBuffers();
    void ClearLines();
    void Cleanup();
};

#endif // !DEBUG_SYSTEM_H
