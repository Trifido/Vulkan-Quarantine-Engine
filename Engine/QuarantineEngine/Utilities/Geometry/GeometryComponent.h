#pragma once

#ifndef GAEOMETRY_COMPONENT_H
#define GAEOMETRY_COMPONENT_H

#include "GameComponent.h"
#include <Geometry/Vertex.h>
#include <DeviceModule.h>
#include <Geometry/PrimitiveTypes.h>

class GeometryComponent : GameComponent
{
protected:
    VkDeviceMemory  vertexBufferMemory = VK_NULL_HANDLE;
    VkDeviceMemory  indexBufferMemory = VK_NULL_HANDLE;

    uint32_t    numAttributes;

    void createIndexBuffer();
    void createComputeBuffer();
    virtual void createVertexBuffer() = 0;

public:
    static DeviceModule* deviceModule_ptr;
    VkBuffer vertexBuffer;
    VkBuffer indexBuffer;
    uint32_t numVertices;
    uint32_t numFaces;
    std::vector<uint32_t> indices;

    virtual void InitializeMesh(size_t numAttributes) = 0;
    static VkVertexInputBindingDescription  getBindingDescription(bool hasAnimation);
    static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions(bool hasAnimation);// = false
    void cleanup();
};

#endif
