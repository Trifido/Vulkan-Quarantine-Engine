#include "Meshlet.h"

void Meshlet::GenerateMeshlet(const std::vector<PBRVertex>& vertices, const std::vector<uint32_t>& indices)
{
    size_t max_meshlets;
    size_t meshlets_count;
    std::vector<meshopt_Meshlet> meshlets;
    std::vector<unsigned int> meshlet_vertices_indices;
    std::vector<unsigned char> meshlet_triangles;

    this->verticesData = vertices;
    this->indexData = indices;

    max_meshlets = meshopt_buildMeshletsBound(indices.size(), this->MAX_VERTICES, this->MAX_TRIANGLES);
    meshlets = std::vector<meshopt_Meshlet>(max_meshlets);
    meshlet_vertices_indices = std::vector<unsigned int>(max_meshlets * this->MAX_VERTICES);
    meshlet_triangles = std::vector<unsigned char>(max_meshlets * this->MAX_TRIANGLES * 3);

     meshlets_count = meshopt_buildMeshlets(
        meshlets.data(),
        meshlet_vertices_indices.data(),
        meshlet_triangles.data(),
        indices.data(),
        indices.size(),
        &vertices[0].pos.x,
        vertices.size(),
        sizeof(PBRVertex),
        this->MAX_VERTICES,
        this->MAX_TRIANGLES,
        this->CONE_WEIGHT);
    meshlets.resize(meshlets_count);

    uint32_t meshlet_vertex_offset = meshlet_vertices_indices.size();
    for (size_t i = 0; i < meshlets_count; ++i)
    {
        const meshopt_Meshlet& local_meshlet = meshlets[i];

        meshopt_Bounds bounds = meshopt_computeMeshletBounds(
                                    &meshlet_vertices_indices[local_meshlet.vertex_offset],
                                    &meshlet_triangles[local_meshlet.triangle_offset],
                                    local_meshlet.triangle_count,
                                    &vertices[0].pos.x,
                                    vertices.size(),
                                    sizeof(PBRVertex)
        );

        MeshletGPU newMeshlet = {};
        newMeshlet.data_offset = this->meshletData.size();
        newMeshlet.vertex_count = local_meshlet.vertex_count;
        newMeshlet.triangle_count = local_meshlet.triangle_count;
        newMeshlet.center = glm::vec3(bounds.center[0], bounds.center[1], bounds.center[2]);
        newMeshlet.radius = bounds.radius;

        newMeshlet.cone_axis = glm::vec3(bounds.cone_axis[0], bounds.cone_axis[1], bounds.cone_axis[2]);
        newMeshlet.cone_apex = glm::vec3(bounds.cone_apex[0], bounds.cone_apex[1], bounds.cone_apex[2]);

        newMeshlet.cone_cutoff = bounds.cone_cutoff;
        newMeshlet.mesh_index = 0;


        const uint32_t index_group_count = (local_meshlet.triangle_count * 3 + 3) / 4;
        this->meshletData.reserve(this->meshletData.size() + local_meshlet.vertex_count + index_group_count);

        for (uint32_t i = 0; i < newMeshlet.vertex_count; ++i)
        {
            uint32_t vertex_index = meshlet_vertex_offset + meshlet_vertices_indices[local_meshlet.vertex_offset + i];
            this->meshletData.push_back(vertex_index);
        }

        const uint32_t* index_groups = reinterpret_cast<const uint32_t*>(meshlet_triangles.data() + local_meshlet.triangle_offset);
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
