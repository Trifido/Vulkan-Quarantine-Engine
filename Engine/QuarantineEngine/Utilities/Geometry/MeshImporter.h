#pragma once
#ifndef MESH_IMPORTER_H
#define MESH_IMPORTER_H

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <vector>
#include <Geometry/Mesh.h>
#include <CustomTexture.h>

class MeshImporter 
{
private:
    const aiScene* scene;
    std::vector<aiNode*> ai_nodes;
public:
    std::vector<Mesh> meshes;

private:
    void LoadMesh(std::string path);
    void ProcessNode(aiNode* node, const aiScene* scene);
    Mesh ProcessMesh(aiMesh* mesh, const aiScene* scene);
    std::vector<CustomTexture> LoadMaterialTextures(aiMaterial* mat, aiTextureType type);

public:
    MeshImporter();
    MeshImporter(std::string pathfile);
};

#endif // !MESH_H
