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
typedef struct AnimationVertexData AnimationVertexData;

class Mesh : public GeometryComponent
{
private:
    std::string PATH;
    std::vector<Vertex> vertices;
    std::vector<AnimationVertexData> animationData;
    bool hasAnimationData = false;

public:
    bool IsMeshletEnabled = false;

private:
    void createVertexBuffer() override;
    void createAnimationVertexBuffer();

public:
    Mesh(const MeshData& data);
    void InitializeMesh() override;
};

#endif // !MESH_H
