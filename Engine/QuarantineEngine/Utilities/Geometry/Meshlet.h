#pragma once

#ifndef MESHLET_H
#define MESHLET_H

#include <vector>
#include <meshoptimizer.h>
#include <Vertex.h>

class Meshlet
{
private:
    const size_t MAX_VERTICES = 64;
    const size_t MAX_TRIANGLES = 124;
    const float CONE_WEIGHT = 0.0f;

    size_t max_meshlets;
    size_t meshlets_count;
    std::vector<meshopt_Meshlet> meshlets;
    std::vector<unsigned int> meshlet_vertices;
    std::vector<unsigned char> meshlet_triangles;
    std::vector<VertexMeshlet> meshletVertices;

public:
    void GenerateMeshlet(const std::vector<PBRVertex>& vertices, const std::vector<uint32_t>& indices);
};

#endif // !MESHLET_H
