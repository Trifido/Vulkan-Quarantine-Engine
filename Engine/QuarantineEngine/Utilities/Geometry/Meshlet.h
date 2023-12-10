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

class Meshlet
{
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
};

#endif // !MESHLET_H
