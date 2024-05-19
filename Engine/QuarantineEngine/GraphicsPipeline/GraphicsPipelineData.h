#pragma once

#ifndef GRAPHICS_PIPELINE_DATA_H
#define GRAPHICS_PIPELINE_DATA_H

#include <vulkan/vulkan.h>
#include <Vertex.h>
#include <ShadowMappingMode.h>

struct GraphicsPipelineData
{
    VkRenderPass renderPass = VK_NULL_HANDLE;
    VkPolygonMode polygonMode = VkPolygonMode::VK_POLYGON_MODE_FILL;
    VkPrimitiveTopology topology = VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    uint32_t vertexBufferStride = sizeof(Vertex);
    float lineWidth = 1.0f;
    bool HasVertexData = true;
    bool IsMeshShader = false;
    ShadowMappingMode shadowMode = ShadowMappingMode::NONE;

    GraphicsPipelineData() {}
};

#endif // !GRAPHICS_PIPELINE_DATA_H
