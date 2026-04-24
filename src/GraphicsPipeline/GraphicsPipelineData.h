#pragma once

#ifndef GRAPHICS_PIPELINE_DATA_H
#define GRAPHICS_PIPELINE_DATA_H

#include <vulkan/vulkan.h>
#include <Vertex.h>
#include <ShadowMappingMode.h>

struct GraphicsPipelineData
{
    std::shared_ptr<VkRenderPass> renderPass;
    VkPolygonMode polygonMode = VkPolygonMode::VK_POLYGON_MODE_FILL;
    VkPrimitiveTopology topology = VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    VkCullModeFlags cullMode = VK_CULL_MODE_BACK_BIT;
    VkFrontFace frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    uint32_t vertexBufferStride = sizeof(Vertex);
    float lineWidth = 1.0f;
    bool HasVertexData = true;
    bool IsMeshShader = false;
    bool DepthTestEnabled = true;
    bool DepthWriteEnabled = true;
    ShadowMappingMode shadowMode = ShadowMappingMode::NONE;

    GraphicsPipelineData() {}
};

#endif // !GRAPHICS_PIPELINE_DATA_H
