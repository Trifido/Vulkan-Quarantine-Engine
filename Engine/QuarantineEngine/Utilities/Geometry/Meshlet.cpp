#include "Meshlet.h"
#include <time.h>
#include <iostream>

void Meshlet::GenerateMeshlet(const std::vector<PBRVertex>& vertices, const std::vector<uint32_t>& indices)
{
    //this->meshletVertices.reserve(vertices.size());

    //for each (auto vertex in vertices)
    //{
    //    this->meshletVertices.push_back(VertexMeshlet(vertex));
    //}

    this->max_meshlets = meshopt_buildMeshletsBound(indices.size(), this->MAX_VERTICES, this->MAX_TRIANGLES);
    this->meshlets = std::vector<meshopt_Meshlet>(this->max_meshlets);
    this->meshlet_vertices = std::vector<unsigned int>(this->max_meshlets * this->MAX_VERTICES);
    this->meshlet_triangles = std::vector<unsigned char>(this->max_meshlets * this->MAX_TRIANGLES * 3);

    double startc = double(clock()) / double(CLOCKS_PER_SEC);

    //meshlets.resize(meshopt_buildMeshlets(
    //    &meshlets[0],
    //    &meshlet_vertices[0],
    //    &meshlet_triangles[0], &indices[0],
    //    indices.size(),
    //    &this->meshletVertices[0].pos.x,
    //    this->meshletVertices.size(),
    //    sizeof(VertexMeshlet),
    //    this->MAX_VERTICES,
    //    this->MAX_TRIANGLES,
    //    this->CONE_WEIGHT));

     this->meshlets_count = meshopt_buildMeshlets(
        &meshlets[0],
        &meshlet_vertices[0],
        &meshlet_triangles[0], &indices[0],
        indices.size(),
        &vertices[0].pos.x,
        vertices.size(),
        sizeof(PBRVertex),
        this->MAX_VERTICES,
        this->MAX_TRIANGLES,
        this->CONE_WEIGHT);
    meshlets.resize(this->meshlets_count);

    double endc = double(clock()) / double(CLOCKS_PER_SEC);

    std::cout << ("Time: %d", endc - startc) << std::endl;

    if (this->meshlets_count)
    {
        const meshopt_Meshlet& last = meshlets.back();
        meshlet_vertices.resize(last.vertex_offset + last.vertex_count);
        meshlet_triangles.resize(last.triangle_offset + ((last.triangle_count * 3 + 3) & ~3));
    }

    for (size_t i = 0; i < this->meshlets_count; ++i)
    {
        const meshopt_Meshlet& m = meshlets[i];
        meshopt_Bounds bounds = meshopt_computeMeshletBounds(&meshlet_vertices[m.vertex_offset], &meshlet_triangles[m.triangle_offset], m.triangle_count, &vertices[0].pos.x, vertices.size(), sizeof(PBRVertex));
    }
}
