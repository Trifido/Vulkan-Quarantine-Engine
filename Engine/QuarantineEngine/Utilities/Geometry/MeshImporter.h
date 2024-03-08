#pragma once
#ifndef MESH_IMPORTER_H
#define MESH_IMPORTER_H

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <vector>
#include <map>
#include <string>
#include <Geometry/Mesh.h>
#include <MaterialManager.h>
#include <TextureManager.h>
#include <AnimationResources.h>

struct AnimationVertexData
{
    int boneIDs[4];
    float boneWeights[4];
};

struct MeshData
{
    std::string name;
    size_t numVertices = 0;
    size_t numFaces = 0;
    size_t numIndices = 0;
    std::vector<Vertex> vertices;
    std::vector<AnimationVertexData> animationData;
    std::vector<unsigned int> indices;
    std::string materialID = "default";
    glm::mat4 model = glm::mat4(1.0);
    bool HasAnimation = false;
};

class MeshImporter 
{
private:
    const aiScene* scene;
    MaterialManager* materialManager;
    std::string meshPath;
    std::string texturePath;
    std::string fileExtension;
    std::unordered_map<std::string, BoneInfo> m_BoneInfoMap;
    size_t numBones = 0;
    bool hasAnimation = false;

    glm::vec3 aabbMin;
    glm::vec3 aabbMax;

private:
    MeshData ProcessMesh(aiMesh* mesh, const aiScene* scene);
    void ProcessNode(aiNode* node, const aiScene* scene, glm::mat4 parentTransform, std::vector<MeshData> &meshes);
    glm::mat4 GetGLMMatrix(aiMatrix4x4 transform);
    void ProcessMaterial(aiMesh* mesh, const aiScene* scene, MeshData& meshData);
    void CheckPaths(std::string path);
    void SetVertexBoneDataToDefault(AnimationVertexData& animData);
    void SetVertexBoneData(AnimationVertexData& animData, int boneID, float weight);
    void ExtractBoneWeightForVertices(MeshData& data, aiMesh* mesh);
    void RemapGeometry(MeshData& data);
    void ComputeAABB(const glm::vec4 & coord);

public:
    bool EnableMeshShaderMaterials = false;

public:
    MeshImporter();
    std::vector<MeshData> LoadMesh(std::string path);
    static MeshData LoadRawMesh(float rawData[], unsigned int numData, unsigned int offset);
    static void RecreateNormals(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices);
    static void RecreateTangents(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices);
    auto& GetBoneInfoMap() { return m_BoneInfoMap; }
    size_t& GetBoneCount() { return numBones; }
    inline bool HasAnimation() { return hasAnimation; }
    std::pair<glm::vec3, glm::vec3> GetAABBData();
};

#endif // !MESH_H
