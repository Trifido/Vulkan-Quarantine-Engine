#pragma once
#ifndef MESH_H
#define MESH_H

#include <vulkan/vulkan.hpp>
#include <string>
#include "GeometryComponent.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <MeshImporter.h>
#include <Meshlet.h>

typedef struct MeshData MeshData;

class Mesh : public GeometryComponent
{
private:
    std::string PATH;
    std::vector<PBRVertex> vertices;

public:
    bool IsMeshletEnabled = false;

private:
    void createVertexBuffer() override;

public:
    Mesh(const MeshData& data);
    void InitializeMesh(size_t numAttributes) override;
};

#endif // !MESH_H
