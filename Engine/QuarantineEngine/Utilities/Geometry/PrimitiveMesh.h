#pragma once

#ifndef PRIMITIVE_MESH_H
#define PRIMITIVE_MESH_H

#include <vulkan/vulkan.hpp>
#include "GeometryComponent.h"
#include <Geometry/PrimitiveTypes.h>

class PrimitiveMesh : public GeometryComponent
{
public:
    PRIMITIVE_TYPE type;
    //std::vector<PrimitiveVertex> vertices;
    std::vector<PBRVertex> vertices;

    PrimitiveMesh();
    PrimitiveMesh(PRIMITIVE_TYPE type);
    void InitializeMesh() override;
    VkVertexInputBindingDescription getBindingDescription() override;
    std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions() override;

private:
    void InitializePlane();
    void InitializeCube();
    void createVertexBuffer() override;
};

#endif
