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

    PrimitiveMesh();
    PrimitiveMesh(PRIMITIVE_TYPE type);
    void InitializeMesh() override;
    VkVertexInputBindingDescription getBindingDescription() override;
    std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions() override;

private:
    void InitializePlane();
};

#endif
