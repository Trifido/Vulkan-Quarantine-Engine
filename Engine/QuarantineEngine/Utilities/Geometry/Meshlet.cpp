#include "Meshlet.h"

void Meshlet::GenerateMeshlet(const std::vector<PBRVertex>& vertices, const std::vector<uint32_t>& indices)
{
    size_t max_meshlets;
    size_t meshlets_count;
    std::vector<meshopt_Meshlet> meshlets;
    std::vector<unsigned int> meshlet_vertices;
    std::vector<unsigned char> meshlet_triangles;

    this->verticesData = vertices;

    max_meshlets = meshopt_buildMeshletsBound(indices.size(), this->MAX_VERTICES, this->MAX_TRIANGLES);
    meshlets = std::vector<meshopt_Meshlet>(max_meshlets);
    meshlet_vertices = std::vector<unsigned int>(max_meshlets * this->MAX_VERTICES);
    meshlet_triangles = std::vector<unsigned char>(max_meshlets * this->MAX_TRIANGLES * 3);

     meshlets_count = meshopt_buildMeshlets(
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
    meshlets.resize(meshlets_count);

    if (meshlets_count)
    {
        const meshopt_Meshlet& last = meshlets.back();
        meshlet_vertices.resize(last.vertex_offset + last.vertex_count);
        meshlet_triangles.resize(last.triangle_offset + ((last.triangle_count * 3 + 3) & ~3));
    }

    uint32_t meshlet_vertex_offset = meshlet_vertices.size();
    for (size_t i = 0; i < meshlets_count; ++i)
    {
        const meshopt_Meshlet& m = meshlets[i];
        meshopt_Bounds bounds = meshopt_computeMeshletBounds(&meshlet_vertices[m.vertex_offset], &meshlet_triangles[m.triangle_offset], m.triangle_count, &vertices[0].pos.x, vertices.size(), sizeof(PBRVertex));

        MeshletGPU newMeshlet = {};
        newMeshlet.data_offset = this->meshletData.size();
        newMeshlet.vertex_count = m.vertex_count;
        newMeshlet.triangle_count = m.triangle_count;
        newMeshlet.center = glm::vec3(bounds.center[0], bounds.center[1], bounds.center[2]);
        newMeshlet.radius = bounds.radius;

        newMeshlet.cone_axis[0] = bounds.cone_axis_s8[0];
        newMeshlet.cone_axis[1] = bounds.cone_axis_s8[1];
        newMeshlet.cone_axis[2] = bounds.cone_axis_s8[2];

        newMeshlet.cone_cutoff = bounds.cone_cutoff_s8;
        newMeshlet.mesh_index = 0;


        const uint32_t index_group_count = (m.triangle_count * 3 + 3) / 4;
        this->meshletData.reserve(this->meshletData.size() + m.vertex_count + index_group_count);

        for (uint32_t i = 0; i < newMeshlet.vertex_count; ++i)
        {
            uint32_t vertex_index = meshlet_vertex_offset + meshlet_vertices[m.vertex_offset + i];
            this->meshletData.push_back(vertex_index);
        }

        const uint32_t* index_groups = reinterpret_cast<const uint32_t*>(meshlet_triangles.data() + m.triangle_offset);
        for (uint32_t i = 0; i < index_group_count; ++i)
        {
            const uint32_t index_group = index_groups[i];
            this->meshletData.push_back(index_group);
        }

        this->gpuMeshlets.push_back(newMeshlet);
    }

    while (gpuMeshlets.size() % 32)
    {
        gpuMeshlets.push_back(MeshletGPU());
    }
}
