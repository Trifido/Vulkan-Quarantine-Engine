#pragma once
#ifndef MESH_IMPORTER_H
#define MESH_IMPORTER_H

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <vector>
#include <Geometry/Mesh.h>
#include <CustomTexture.h>

struct MeshData
{
    std::string name;
    size_t numVertices;
    size_t numFaces;
    size_t numIndices;
    size_t numPositions;
    std::vector<PBRVertex> vertices;
    std::vector<unsigned int> indices;

    glm::mat4 model = glm::mat4(1.0);
};

class MeshImporter 
{
private:
    MeshData ProcessMesh(aiMesh* mesh, const aiScene* scene);
    void  ProcessNode(aiNode* node, const aiScene* scene, glm::mat4 parentTransform, std::vector<MeshData> &meshes);
    //void ProcessTextures(aiMesh* mesh, const aiScene* scene);
    //std::vector<CustomTexture> LoadMaterialTextures(aiMaterial* mat, aiTextureType type);
    glm::mat4 GetGLMMatrix(aiMatrix4x4 transform);

public:
    MeshImporter();
    std::vector<MeshData> LoadMesh(std::string path);
    static MeshData LoadRawMesh(float rawData[], unsigned int numData, unsigned int offset);
    static void RecreateNormals(std::vector<PBRVertex>& vertices, std::vector<unsigned int>& indices);
    static void RecreateTangents(std::vector<PBRVertex>& vertices, std::vector<unsigned int>& indices);
};

#endif // !MESH_H
