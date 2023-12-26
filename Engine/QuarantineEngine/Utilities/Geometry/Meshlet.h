#pragma once

#ifndef MESHLET_H
#define MESHLET_H

#include <vector>
#include <meshoptimizer.h>
#include <Vertex.h>

struct MeshletGPU
{
    glm::vec3 center;
    float radius;
    glm::vec3 cone_axis;
    float cone_cutoff;
    glm::vec3 cone_apex;
    uint32_t data_offset;
    uint32_t mesh_index;
    uint32_t vertex_count;
    uint32_t triangle_count;
    uint32_t auxiliar;
};

struct BoundingSphere
{
    alignas(16) glm::vec3 center;
    alignas(4) float radius;
};

struct BoundingCone
{
    alignas(16) glm::vec3 normal;
    alignas(4) float angle;

    static BoundingCone GetApproximateReflexBoundingCone(
        std::vector<glm::vec4*>& normals);
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

struct MeshletDescriptor
{
    BoundingSphere sphere;
    BoundingCone cone;
    uint16_t primitive_count;
};

class Meshlet
{
private:
    const uint16_t meshlet_primitive_count = 32;
public:
    std::vector<MeshletDescriptor> meshlet_descriptors;

private:
    const size_t MAX_VERTICES = 64;
    const size_t MAX_TRIANGLES = 124;
    const float CONE_WEIGHT = 0.5f;

public:
    std::vector<MeshletGPU> gpuMeshlets;
    std::vector<uint32_t> meshletData;
    std::vector<PBRVertex> verticesData;
    std::vector<uint32_t> indexData;

public:
    void GenerateMeshlet(const std::vector<PBRVertex>& vertices, const std::vector<uint32_t>& indices);
    void GenerateCustomMeshlet(std::vector<PBRVertex>& vertices, std::vector<uint32_t>& indices);
};

#endif // !MESHLET_H
