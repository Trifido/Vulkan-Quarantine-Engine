#pragma once

#ifndef GRAPHICS_PIPELINE_DATA_H
#define GRAPHICS_PIPELINE_DATA_H

#include <vulkan/vulkan.h>
#include <Vertex.h>

struct GraphicsPipelineData
{
    VkPolygonMode polygonMode = VkPolygonMode::VK_POLYGON_MODE_FILL;
    VkPrimitiveTopology topology = VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    uint32_t vertexBufferStride = sizeof(PBRVertex);
    bool HasVertexData = true;

    GraphicsPipelineData() {}
};

#endif // !GRAPHICS_PIPELINE_DATA_H