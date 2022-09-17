#pragma once

#ifndef GAEOMETRYCOMPONENT_H
#define GAEOMETRYCOMPONENT_H

#include "GameComponent.h"
#include <Vertex.h>
#include <DeviceModule.h>
#include <Geometry/PrimitiveTypes.h>

class GeometryComponent : GameComponent
{
protected:
    VkDeviceMemory  vertexBufferMemory;
    VkDeviceMemory  indexBufferMemory;

    uint32_t    numAttributes;

    void createVertexBuffer();
    void createIndexBuffer();

public:
    static DeviceModule* deviceModule_ptr;
    VkBuffer vertexBuffer;
    VkBuffer indexBuffer;
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    uint32_t numVertices;
    uint32_t numFaces;

    virtual void InitializeMesh() = 0;
    virtual VkVertexInputBindingDescription  getBindingDescription() = 0;
    virtual std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions() = 0;
    void cleanup();
};

#endif
