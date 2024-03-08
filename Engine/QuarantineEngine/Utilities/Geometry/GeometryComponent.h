#pragma once

#ifndef GAEOMETRY_COMPONENT_H
#define GAEOMETRY_COMPONENT_H

#include "GameComponent.h"
#include <Geometry/Vertex.h>
#include <DeviceModule.h>
#include <Geometry/PrimitiveTypes.h>
#include <Meshlet.h>

class GeometryComponent : GameComponent
{
protected:
    VkDeviceMemory  vertexBufferMemory = VK_NULL_HANDLE;
    VkDeviceMemory  indexBufferMemory = VK_NULL_HANDLE;
    VkDeviceMemory  animationBufferMemory = VK_NULL_HANDLE;

    uint32_t    numAttributes;

    void createIndexBuffer();
    virtual void createVertexBuffer() = 0;

public:
    static DeviceModule* deviceModule_ptr;
    VkBuffer vertexBuffer = VK_NULL_HANDLE;
    VkBuffer indexBuffer = VK_NULL_HANDLE;
    VkBuffer animationBuffer = VK_NULL_HANDLE;
    uint32_t numVertices;
    uint32_t numFaces;
    std::vector<uint32_t> indices;
    std::shared_ptr<Meshlet> meshlets_ptr = nullptr;

    virtual void InitializeMesh(size_t attributesSize) = 0;
    virtual void cleanup();
};

#endif
