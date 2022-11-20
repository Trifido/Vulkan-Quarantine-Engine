#pragma once
#ifndef MESH_IMPORTER_H
#define MESH_IMPORTER_H

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <vector>
#include <Geometry/Mesh.h>
#include <MaterialManager.h>
#include <TextureManager.h>
#include <set>
#include <AnimationResources.h>


struct MeshData
{
    std::string name;
    size_t numVertices = 0;
    size_t numFaces = 0;
    size_t numIndices = 0;
    size_t numPositions = 0;
    std::vector<PBRVertex> vertices;
    std::vector<unsigned int> indices;
    std::string materialID = "default";
    glm::mat4 model = glm::mat4(1.0);
};

class MeshImporter 
{
private:
    const aiScene* scene;
    MaterialManager* materialManager;
    TextureManager* textureManager;
    std::string meshPath;
    std::string texturePath;
    std::string fileExtension;
    std::set<std::string> currentTextures;
    std::map<std::string, BoneInfo> m_BoneInfoMap;
    size_t numBones = 0;
    bool hasAnimation = false;

private:
    MeshData ProcessMesh(aiMesh* mesh, const aiScene* scene);
    void ProcessNode(aiNode* node, const aiScene* scene, glm::mat4 parentTransform, std::vector<MeshData> &meshes);
    glm::mat4 GetGLMMatrix(aiMatrix4x4 transform);
    void ProcessMaterial(aiMesh* mesh, const aiScene* scene, MeshData& meshData);
    std::string GetTexture(aiMaterial* mat, aiTextureType type, TEXTURE_TYPE textureType);
    void CheckPaths(std::string path);
    void SetVertexBoneDataToDefault(PBRVertex& vertex);
    void SetVertexBoneData(PBRVertex& vertex, int boneID, float weight);
    void ExtractBoneWeightForVertices(MeshData& data, aiMesh* mesh, const aiScene* scene);

public:
    MeshImporter();
    std::vector<MeshData> LoadMesh(std::string path);
    static MeshData LoadRawMesh(float rawData[], unsigned int numData, unsigned int offset);
    static void RecreateNormals(std::vector<PBRVertex>& vertices, std::vector<unsigned int>& indices);
    static void RecreateTangents(std::vector<PBRVertex>& vertices, std::vector<unsigned int>& indices);
    auto& GetBoneInfoMap() { return m_BoneInfoMap; }
    size_t& GetBoneCount() { return numBones; }
    inline bool HasAnimation() { return hasAnimation; }
};

#endif // !MESH_H
