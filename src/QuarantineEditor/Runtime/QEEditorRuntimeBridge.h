#pragma once

#include <vulkan/vulkan.h>

namespace QEEditorRuntimeBridge
{
    void SetupEditorRuntime(bool showGrid);
    void SetEditorGridVisible(bool visible);
    void DrawEditorObjects(VkCommandBuffer& commandBuffer, uint32_t frameIndex);
    void CleanupEditorRuntime();
}
