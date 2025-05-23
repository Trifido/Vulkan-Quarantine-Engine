#pragma once
#ifndef MESH_IMPORTER_H
#define MESH_IMPORTER_H

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <filesystem>
#include <vector>
#include <map>
#include <string>
#include <MaterialManager.h>
#include <ShaderManager.h>
#include <TextureManager.h>
#include <AnimationResources.h>
#include <QEMeshData.h>

namespace fs = std::filesystem;

class MeshImporter 
{
private:
    static QEMeshData ProcessMesh(aiMesh* mesh, const aiScene* scene, std::unordered_map<std::string, BoneInfo>& m_BoneInfoMap);
    static void ProcessNode(aiNode* node, const aiScene* scene, glm::mat4 parentTransform, QEMesh& mesh, const fs::path& matpath);
    static glm::mat4 GetGLMMatrix(aiMatrix4x4 transform);
    static void ProcessMaterial(aiMesh* mesh, const aiScene* scene, QEMeshData& meshData, const fs::path& matpath);
    static void SetVertexBoneDataToDefault(AnimationVertexData& animData);
    static void SetVertexBoneData(AnimationVertexData& animData, int boneID, float weight);
    static void ExtractBoneWeightForVertices(QEMeshData& data, aiMesh* mesh, std::unordered_map<std::string, BoneInfo>& m_BoneInfoMap);
    static void RemapGeometry(QEMeshData& data);
    static void ComputeAABB(const glm::vec4 & coord, std::pair<glm::vec3, glm::vec3> &AABBData);

public:
    static QEMesh LoadMesh(std::string path);
    static QEMeshData LoadRawMesh(float rawData[], unsigned int numData, unsigned int offset);
    static void RecreateNormals(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices);
    static void RecreateTangents(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices);

private:
    static std::string GetTextureTypeName(aiTextureType type);
    static void SaveTextureToFile(const aiTexture* texture, const std::string& outputPath);
    static void CopyTextureFile(const std::string& sourcePath, const std::string& destPath);
    static void ExtractAndUpdateTextures(aiScene* scene, const std::string& outputTextureFolder, const std::string& modelDirectory);
    static void ExtractAndUpdateMaterials(aiScene* scene, const std::string& outputTextureFolder, const std::string& outputMaterialPath, const std::string& modelDirectory);
    static void RemoveOnlyEmbeddedTextures(aiScene* scene);
public:
    static bool LoadAndExportModel(const std::string& inputPath, const std::string& outputMeshPath, const std::string& outputMaterialPath, const std::string& outputTexturePath);
};

#endif // !MESH_H
