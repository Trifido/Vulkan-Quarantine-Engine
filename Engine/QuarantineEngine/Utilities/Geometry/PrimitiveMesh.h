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
    std::vector<Vertex> vertices;
    std::pair<glm::vec3, glm::vec3> aabbData;

    PrimitiveMesh();
    PrimitiveMesh(PRIMITIVE_TYPE type);
    void InitializeMesh() override;

private:
    void InitializePoint();
    void InitializeTriangle();
    void InitializePlane();
    void InitializeFloorPlane();
    void InitializeGrid();
    void InitializeCube();
    void InitializeSphere();
    void createVertexBuffer() override;
};

#endif
