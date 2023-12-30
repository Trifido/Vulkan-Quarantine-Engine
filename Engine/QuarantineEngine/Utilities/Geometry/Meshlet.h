#pragma once

#ifndef MESHLET_H
#define MESHLET_H

#include <vector>
#include <meshoptimizer.h>
#include <Vertex.h>

struct BoundingSphere
{
    alignas(16) glm::vec3 center;
    alignas(4) float radius;
};

struct BoundingCone
{
    alignas(16) glm::vec3 normal;
    alignas(4) float angle;
    glm::vec4 position;

    static BoundingCone GetApproximateReflexBoundingCone(
        std::vector<glm::vec4*>& normals);
};

struct MeshletDescriptor
{
    BoundingSphere sphere;
    BoundingCone cone;
    uint16_t primitive_count;
};

class AxisAlignedBoundingBox
{
public:
    explicit AxisAlignedBoundingBox(
        std::vector<glm::vec4*>& vertices);
    const glm::vec3& getCenter() const { return _center; };
    const glm::vec3& getExtents() const { return _extents; };
    const glm::vec3& getMin() const { return _min; };
    const glm::vec3& getMax() const { return _max; };

private:
    glm::vec3 _center{};
    glm::vec3 _extents{};

    glm::vec3 _min{};
    glm::vec3 _max{};
};

class Meshlet
{
private:
    const uint16_t meshlet_primitive_count = 32;

private:
    const size_t MAX_VERTICES = 96;
    const size_t MAX_TRIANGLES = 32;
    const float CONE_WEIGHT = 0.5f;

public:
    std::vector<MeshletDescriptor> gpuMeshlets;
    std::vector<uint32_t> meshletData;
    std::vector<PBRVertex> verticesData;
    std::vector<uint32_t> indexData;

public:
    void GenerateMeshlet(const std::vector<PBRVertex>& vertices, const std::vector<uint32_t>& indices);
    void GenerateCustomMeshlet(std::vector<PBRVertex>& vertices, std::vector<uint32_t>& indices);
};

#endif // !MESHLET_H
