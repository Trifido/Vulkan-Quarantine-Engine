#pragma once

#ifndef QE_MESH_DATA
#define QE_MESH_DATA

#include <string>
#include <vector>
#include <Vertex.h>

struct AnimationVertexData
{
    int boneIDs[4];
    float boneWeights[4];
};

struct QEMeshData
{
    size_t NumVertices = 0;
    size_t NumFaces = 0;
    size_t NumIndices = 0;
    std::vector<Vertex> Vertices;
    std::vector<AnimationVertexData> AnimationData;
    std::vector<unsigned int> Indices;
    std::string MaterialID = "default";
    glm::mat4 ModelTransform = glm::mat4(1.0);
    bool HasAnimation = false;
    std::pair<glm::vec3, glm::vec3> BoundingBox;
};

struct QEMesh  
{  
    std::string Name = "";  
    std::string FilePath = "";  
    std::vector<QEMeshData> MeshData;  
    std::pair<glm::vec3, glm::vec3> BoundingBox;
    std::unordered_map<std::string, BoneInfo> BonesInfoMap;
    std::vector<AnimationData> AnimationData;

    QEMesh() = default;

    QEMesh(std::string name, std::string filePath, std::vector<QEMeshData> meshData)  
    {  
        Name = name;  
        FilePath = filePath;  
        MeshData = meshData;  

        if (!meshData.empty())
        {
            BoundingBox = meshData[0].BoundingBox;

            for (const auto& mesh : meshData)  
            {  
                BoundingBox.first = glm::min(BoundingBox.first, mesh.BoundingBox.first);  
                BoundingBox.second = glm::max(BoundingBox.second, mesh.BoundingBox.second);  
            }  
        }
    }  
};

#endif // QE_MESH_DATA
