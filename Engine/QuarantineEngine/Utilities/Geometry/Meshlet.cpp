#include "Meshlet.h"
#include <map>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

BoundingCone BoundingCone::GetApproximateReflexBoundingCone(
    std::vector<glm::vec4*>& normals)
{
    glm::vec3 mean_normal{};
    for (auto normal : normals)
    {
        mean_normal += glm::vec3(*normal);
    }
    mean_normal = glm::normalize(mean_normal);

    float angular_span = 0.0f;
    for (auto normal : normals)
    {
        angular_span =
            glm::max(angular_span,
                glm::acos(glm::dot(mean_normal, glm::vec3(*normal))));
    }

    BoundingCone cone;
    cone.normal = mean_normal;
    cone.angle = angular_span;
    return cone;
}

AxisAlignedBoundingBox::AxisAlignedBoundingBox(
    std::vector<glm::vec4*>& vertices)
{
    _min = *vertices[0];
    _max = _min;
    for (auto vertex : vertices)
    {
        _min = glm::min(_min, glm::vec3(*vertex));
        _max = glm::max(_max, glm::vec3(*vertex));
    }
    _extents = (_max - _min) / 2.f;
    _center = _min + _extents;
}

void Meshlet::GenerateCustomMeshlet(std::vector<PBRVertex>& vertices, std::vector<uint32_t>& indices)
{
    const auto max_indices_count = indices.size();
    const auto num_faces = indices.size() / 3;

    this->verticesData = vertices;
    this->indexData = indices;

    const auto meshlet_count = num_faces / this->meshlet_primitive_count +
        (num_faces % this->meshlet_primitive_count != 0);

    this->gpuMeshlets.reserve(meshlet_count);

    auto i_face_from = 0u;
    const unsigned int face_count = num_faces;

    std::map<unsigned, bool> unique_index_map;
    std::vector<glm::vec4*> meshlet_vertices;
    std::vector<glm::vec4*> meshlet_normals;

    for (unsigned i_meshlet = 0; i_meshlet < meshlet_count; ++i_meshlet)
    {
        const auto i_face_to = glm::min(i_face_from + this->meshlet_primitive_count, face_count);
        auto global_index_to = i_face_to * 3;
        auto global_index_from = i_face_from * 3;

        uint16_t primitive_count = i_face_to - i_face_from;
        unique_index_map.clear();

        meshlet_vertices.clear();
        meshlet_vertices.reserve(max_indices_count);

        meshlet_normals.clear();
        meshlet_normals.reserve(max_indices_count);

        // Iterate primitives of this meshlet
        for (unsigned i_face = i_face_from; i_face < i_face_to; ++i_face)
        {
            auto global_index_to = i_face * 3;
            auto init_index = this->indexData.at(global_index_to);

            for (unsigned i_indx = 0; i_indx < 3; ++i_indx)
            {
                unsigned index = this->indexData.at(global_index_to + i_indx);

                // Add to map to test if index already present
                if (unique_index_map
                    .insert(std::make_pair(index, true))
                    .second)
                {
                    auto* vertex = &vertices.at(index).pos;
                    auto* normal = &vertices.at(index).norm;

                    meshlet_vertices.push_back(vertex);
                    meshlet_normals.push_back(normal);
                }
            }
        }

        auto aabb = AxisAlignedBoundingBox(meshlet_vertices);
        BoundingSphere bounding_sphere{
            .center = aabb.getCenter(), .radius = [&]() -> float {
              float radius = 0.f;
              for (auto vertex : meshlet_vertices)
              {
                radius =
                    glm::max(radius, glm::distance(bounding_sphere.center,
                                                   glm::vec3(*vertex)));
              }
              return radius;
            }() };

        BoundingCone bounding_cone =
            BoundingCone::GetApproximateReflexBoundingCone(
                meshlet_normals);

        bounding_cone.position = glm::vec4(bounding_sphere.center, 1.0f);

        gpuMeshlets.push_back(
            MeshletDescriptor{ .sphere = bounding_sphere,
                                   .cone = bounding_cone,
                                   .primitive_count = primitive_count });

        i_face_from += this->meshlet_primitive_count;
    }
}

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
        0.5f);
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

        MeshletDescriptor newMeshlet = {};

        BoundingSphere bounding_sphere{
            .center = glm::vec3(bounds.center[0], bounds.center[1], bounds.center[2]),
            .radius = bounds.radius
        };

        BoundingCone bounding_cone
        {
            .normal = glm::vec3(bounds.cone_axis[0], bounds.cone_axis[1], bounds.cone_axis[2]),
            .angle = bounds.cone_cutoff,
            .position = glm::vec4(bounds.cone_apex[0], bounds.cone_apex[1], bounds.cone_apex[2], 1.0f)
        };

        newMeshlet.cone = bounding_cone;
        newMeshlet.sphere = bounding_sphere;
        newMeshlet.primitive_count = local_meshlet.triangle_count;

        this->gpuMeshlets.push_back(newMeshlet);
    }

    while (gpuMeshlets.size() % 32)
    {
        gpuMeshlets.push_back(MeshletDescriptor());
    }
}
