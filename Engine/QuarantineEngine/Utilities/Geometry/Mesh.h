#pragma once
#ifndef MESH_H
#define MESH_H

#include <vulkan/vulkan.hpp>
#include <string>
#include "GeometryComponent.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

class Mesh : public GeometryComponent
{
private:
    std::string PATH;
    std::vector<Vertex> vertices;

    void createVertexBuffer() override;
public:
    Mesh();
    Mesh(std::string pathfile);
    void InitializeMesh() override;
    VkVertexInputBindingDescription getBindingDescription() override;
    std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions() override;
};

#endif // !MESH_H
