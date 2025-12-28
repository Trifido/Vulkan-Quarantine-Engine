#include "MeshImporter.h"
#include <fstream>
#include <memory>
#include <assimp/Exporter.hpp>
#include <assimp/postprocess.h>
#include <assimp/config.h>
#include <unordered_set>
#include "MaterialData.h"
#include "Material.h"
#include <SanitizerHelper.h>

void MeshImporter::RecreateNormals(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices)
{
    for (size_t v = 0; v < vertices.size(); v++)
    {
        vertices.at(v).Normal = glm::vec4(0.0f);
    }

    for (size_t idTr = 0; idTr < indices.size(); idTr += 3)
    {
        size_t idx0 = indices[idTr];
        size_t idx1 = indices[idTr + 1];
        size_t idx2 = indices[idTr + 2];

        glm::vec3 pos0 = glm::vec3(vertices[idx0].Position);
        glm::vec3 pos1 = glm::vec3(vertices[idx1].Position);
        glm::vec3 pos2 = glm::vec3(vertices[idx2].Position);

        glm::vec3 edge1 = pos1 - pos0;
        glm::vec3 edge2 = pos2 - pos0;
        glm::vec3 crossProduct = glm::cross(edge1, edge2);
        glm::vec4 crossProduct4 = glm::vec4(crossProduct.x, crossProduct.y, crossProduct.z, 0.0f);

        vertices[idx0].Normal += crossProduct4;
        vertices[idx1].Normal += crossProduct4;
        vertices[idx2].Normal += crossProduct4;
    }

    for (size_t v = 0; v < vertices.size(); v++)
    {
        vertices.at(v).Normal = glm::normalize(vertices.at(v).Normal);
    }
}

void MeshImporter::SetVertexBoneDataToDefault(AnimationVertexData& animData)
{
    for (int i = 0; i < 4; i++)
    {
        animData.boneIDs[i] = -1;
        animData.boneWeights[i] = 0.0f;
    }
}

void MeshImporter::SetVertexBoneData(AnimationVertexData& animData, int boneID, float weight)
{
    if (weight == 0.0f)
    {
        return;
    }

    for (int i = 0; i < 4; ++i)
    {
        if (animData.boneWeights[i] == 0.0f)
        {
            animData.boneWeights[i] = weight;
            animData.boneIDs[i] = boneID;

            if (i == 4)
            {
                float sum = animData.boneWeights[0] + animData.boneWeights[1] + animData.boneWeights[2] + animData.boneWeights[3];

                if (sum < 1.0f)
                {
                    float rest = 1.0f - sum;
                    rest = rest / 4.0f;

                    if (rest > 0.0f)
                    {
                        animData.boneWeights[0] += rest;
                        animData.boneWeights[1] += rest;
                        animData.boneWeights[2] += rest;
                        animData.boneWeights[3] += rest;
                    }
                }
            }
            return;
        }
    }
}

void MeshImporter::ExtractBoneWeightForVertices(QEMeshData& data, aiMesh* mesh, std::unordered_map<std::string, BoneInfo>& m_BoneInfoMap)
{
    for (unsigned int boneIndex = 0; boneIndex < mesh->mNumBones; boneIndex++)
    {
        int boneID = -1;
        std::string boneName = mesh->mBones[boneIndex]->mName.C_Str();

        if (m_BoneInfoMap.find(boneName) == m_BoneInfoMap.end())
        {
            glm::mat4 offset = ConvertMatrixToGLMFormat(mesh->mBones[boneIndex]->mOffsetMatrix);
            m_BoneInfoMap[boneName] = { static_cast<uint32_t>(m_BoneInfoMap.size()), offset };
            boneID = (int)m_BoneInfoMap.size() - 1;
        }
        else
        {
            boneID = m_BoneInfoMap[boneName].id;
        }
        assert(boneID != -1);
        auto weights = mesh->mBones[boneIndex]->mWeights;
        int numWeights = mesh->mBones[boneIndex]->mNumWeights;

        if (weights != NULL)
        {
            for (int weightIndex = 0; weightIndex < numWeights; weightIndex++)
            {
                int vertexId = weights[weightIndex].mVertexId;
                float weight = weights[weightIndex].mWeight;

                assert(vertexId <= data.Vertices.size());

                SetVertexBoneData(data.AnimationVertexData[vertexId], boneID, weight);
            }
        }
    }
}

void MeshImporter::RemapGeometry(QEMeshData& data)
{
    std::vector<unsigned int> remap(data.NumIndices);

    std::vector<uint32_t> resultIndices;
    std::vector<Vertex> resultVertices;

    size_t total_vertices = meshopt_generateVertexRemap(&remap[0], &data.Indices[0], data.NumIndices, &data.Vertices[0], data.NumIndices, sizeof(Vertex));

    data.NumVertices = total_vertices;
    resultIndices.resize(data.NumIndices);
    meshopt_remapIndexBuffer(&resultIndices[0], &data.Indices[0], data.NumIndices, &remap[0]);

    resultVertices.resize(total_vertices);
    meshopt_remapVertexBuffer(&resultVertices[0], &data.Vertices[0], data.NumIndices, sizeof(Vertex), &remap[0]);

    data.Indices = resultIndices;
    data.Vertices = resultVertices;
}

void MeshImporter::ComputeAABB(const glm::vec4& coord, std::pair<glm::vec3, glm::vec3>& AABBData)
{
    if (AABBData.first.x > coord.x)
        AABBData.first.x = coord.x;
    if (AABBData.first.y > coord.y)
        AABBData.first.y = coord.y;
    if (AABBData.first.z > coord.z)
        AABBData.first.z = coord.z;

    if (AABBData.second.x < coord.x)
        AABBData.second.x = coord.x;
    if (AABBData.second.y < coord.y)
        AABBData.second.y = coord.y;
    if (AABBData.second.z < coord.z)
        AABBData.second.z = coord.z;
}

void MeshImporter::RecreateTangents(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices)
{
    for (size_t idTr = 0; idTr < indices.size(); idTr += 3)
    {
        size_t idx0 = indices[idTr];
        size_t idx1 = indices[idTr + 1];
        size_t idx2 = indices[idTr + 2];


        glm::vec3 pos0 = glm::vec3(vertices[idx0].Position);
        glm::vec3 pos1 = glm::vec3(vertices[idx1].Position);
        glm::vec3 pos2 = glm::vec3(vertices[idx2].Position);


        glm::vec3 edge1 = pos1 - pos0;
        glm::vec3 edge2 = pos2 - pos0;
        glm::vec2 deltaUV1 = vertices[idx1].UV - vertices[idx0].UV;
        glm::vec2 deltaUV2 = vertices[idx2].UV - vertices[idx0].UV;

        float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

        glm::vec4 tangent;

        tangent.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
        tangent.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
        tangent.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
        tangent.w = 0.0f;
        tangent = glm::normalize(tangent);

        vertices[idx0].Tangent = tangent;
        vertices[idx1].Tangent = tangent;
        vertices[idx2].Tangent = tangent;
    }
}

QEMesh MeshImporter::LoadMesh(std::string path)
{
    fs::path filepath = fs::path(path);
    std::string name = filepath.stem().string();
    fs::path matpath = filepath.parent_path().parent_path() / "Materials";

    QEMesh mesh;
    mesh.Name = name;
    mesh.FilePath = path;

    Assimp::Importer importer;
    (void)importer.SetPropertyInteger(AI_CONFIG_PP_LBW_MAX_WEIGHTS, 4);
    auto scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace | aiProcess_LimitBoneWeights);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        fprintf(stderr, "ERROR::ASSIMP::%s", importer.GetErrorString());
        return mesh;
    }

    bool hasAnimation = scene->HasAnimations();

    if (!hasAnimation)
    {
        scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace | aiProcess_PreTransformVertices | aiProcess_PopulateArmatureData);
    }

    glm::vec3 aabbMax = glm::vec3(-std::numeric_limits<float>::infinity());
    glm::vec3 aabbMin = glm::vec3(std::numeric_limits<float>::infinity());

    glm::mat4 parentTransform = glm::mat4(1.0f);
    ProcessNode(scene->mRootNode, scene, parentTransform, mesh, matpath);

    mesh.MaterialRel.resize(mesh.MeshData.size());
    for (int i = 0; i < mesh.MeshData.size(); i++)
    {
        mesh.MaterialRel[i] = mesh.MeshData[i].MaterialID;
    }

    return mesh;
}

void MeshImporter::ProcessNode(aiNode* node, const aiScene* scene, glm::mat4 parentTransform, QEMesh& mesh, const fs::path& matpath)
{
    glm::mat4 localTransform = GetGLMMatrix(node->mTransformation);
    glm::mat4 currentTransform = glm::identity<glm::mat4>();

    // process all the node's meshes (if any)
    for (unsigned int i = 0; i < node->mNumMeshes; i++)
    {
        QEMeshData result = ProcessMesh(scene->mMeshes[node->mMeshes[i]], scene, mesh.BonesInfoMap);
        result.ModelTransform = currentTransform;

        ProcessMaterial(scene->mMeshes[node->mMeshes[i]], scene, result, matpath);

        mesh.MeshData.push_back(result);
    }

    // then do the same for each of its children
    for (unsigned int i = 0; i < node->mNumChildren; i++)
    {
        ProcessNode(node->mChildren[i], scene, currentTransform, mesh, matpath);
    }
}

QEMeshData MeshImporter::ProcessMesh(aiMesh* mesh, const aiScene* scene, std::unordered_map<std::string, BoneInfo>& m_BoneInfoMap)
{
    QEMeshData data = {};

    bool existTangent = mesh->HasTangentsAndBitangents();
    bool existNormal = mesh->HasNormals();

    std::vector<CustomTexture> textures;

    data.NumVertices = mesh->mNumVertices;
    data.NumFaces = mesh->mNumFaces;
    data.NumIndices = mesh->mNumFaces * 3;

    data.Vertices.resize(data.NumVertices);
    data.AnimationVertexData.resize(data.NumVertices);
    data.HasAnimation = scene->HasAnimations();

    for (unsigned int i = 0; i < data.NumVertices; i++)
    {
        Vertex vertex;
        AnimationVertexData animData;

        SetVertexBoneDataToDefault(animData);

        glm::vec4 vector;
        vector.x = mesh->mVertices[i].x;
        vector.y = mesh->mVertices[i].y;
        vector.z = mesh->mVertices[i].z;
        vector.w = 1.0f;
        vertex.Position = vector;

        ComputeAABB(vertex.Position, data.BoundingBox);

        if (existNormal)
        {
            vector.x = mesh->mNormals[i].x;
            vector.y = mesh->mNormals[i].y;
            vector.z = mesh->mNormals[i].z;
            vector.w = 0.0f;
            vertex.Normal = vector;
        }

        if (existTangent)
        {
            vector.x = mesh->mTangents[i].x;
            vector.y = mesh->mTangents[i].y;
            vector.z = mesh->mTangents[i].z;
            vector.w = 0.0f;
            vertex.Tangent = vector;
        }

        if (mesh->mTextureCoords[0])
        {
            glm::vec2 vec;
            vec.x = mesh->mTextureCoords[0][i].x;
            vec.y = mesh->mTextureCoords[0][i].y;
            vertex.UV = vec;
        }
        else
        {
            vertex.UV = glm::vec2(0.0f, 0.0f);
        }

        data.Vertices.at(i) = vertex;
        data.AnimationVertexData.at(i) = animData;
    }

    // process indices
    data.NumIndices = mesh->mNumFaces * mesh->mFaces[0].mNumIndices;
    data.Indices.reserve(data.NumIndices);

    for (unsigned int i = 0; i < mesh->mNumFaces; i++)
    {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++)
        {
            data.Indices.push_back(face.mIndices[j]);
        }
    }

    if (!existNormal)
    {
        RecreateNormals(data.Vertices, data.Indices);
    }

    if (!existTangent)
    {
        RecreateTangents(data.Vertices, data.Indices);
    }

    ExtractBoneWeightForVertices(data, mesh, m_BoneInfoMap);

    RemapGeometry(data);

    return data;
}

QEMeshData MeshImporter::LoadRawMesh(float rawData[], unsigned int numData, unsigned int offset)
{
    QEMeshData data = {};
    unsigned int NUMCOMP = offset;
    for (unsigned int i = 0; i < numData; i++)
    {
        unsigned int index = i * NUMCOMP;

        Vertex vertex;
        // process vertex positions, normals and texture coordinates
        glm::vec4 vector;
        vector.x = rawData[index];
        vector.y = rawData[index + 1];
        vector.z = rawData[index + 2];
        vector.w = 1.0f;
        vertex.Position = vector;

        vector.x = rawData[index + 3];
        vector.y = rawData[index + 4];
        vector.z = rawData[index + 5];
        vector.w = 0.0f;
        vertex.Normal = vector;

        glm::vec2 vec;
        vec.x = rawData[index + 6];
        vec.y = rawData[index + 7];
        vertex.UV = vec;

        data.Vertices.push_back(vertex);
        data.Indices.push_back(i);
    }

    RecreateTangents(data.Vertices, data.Indices);

    return data;
}

glm::mat4 MeshImporter::GetGLMMatrix(aiMatrix4x4 transform)
{
    glm::mat4 result;

    result[0][0] = transform.a1;
    result[0][1] = transform.a2;
    result[0][2] = transform.a3;
    result[0][3] = transform.a4;

    result[1][0] = transform.b1;
    result[1][1] = transform.b2;
    result[1][2] = transform.b3;
    result[1][3] = transform.b4;

    result[2][0] = transform.c1;
    result[2][1] = transform.c2;
    result[2][2] = transform.c3;
    result[2][3] = transform.c4;

    result[3][0] = transform.d1;
    result[3][1] = transform.d2;
    result[3][2] = transform.d3;
    result[3][3] = transform.d4;

    return result;
}

void MeshImporter::ProcessMaterial(aiMesh* mesh, const aiScene* scene, QEMeshData& meshData, const fs::path& matpath)
{
    if (mesh->mMaterialIndex < 0)
        return;

    aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

    if (material == nullptr)
        return;

    aiReturn ret;//Code which says whether loading something has been successful of not

    aiString rawName;
    ret = material->Get(AI_MATKEY_NAME, rawName);//Get the material name (pass by reference)
    if (ret != AI_SUCCESS) rawName = "";//Failed to find material name so makes var empty

    std::string materialName = rawName.C_Str();
    std::shared_ptr<QEMaterial> mat_ptr;

    auto materialManager = MaterialManager::getInstance();

    if (!materialManager->Exists(materialName))
    {
        fs::path materialPath = matpath / (materialName + ".qemat");

        std::ifstream matfile(materialPath, std::ios::binary);
        if (!matfile.is_open())
        {
            std::cerr << "Error al abrir el material " << materialPath << std::endl;
        }
        else
        {
            MaterialDto matDto = MaterialManager::ReadQEMaterial(matfile);
            matfile.close();

            auto shaderManager = ShaderManager::getInstance();
            auto shader = shaderManager->GetShader(matDto.ShaderPath);
            if (shader == nullptr)
            {
                std::cerr << "Error: Shader not found for material " << materialName << std::endl;
                return;
            }

            matDto.UpdateTexturePaths(matpath);

            mat_ptr = std::make_shared<QEMaterial>(QEMaterial(shader, matDto));
            materialManager->AddMaterial(mat_ptr);
        }
    }
    else
    {
        mat_ptr = materialManager->GetMaterial(materialName);
    }

    meshData.MaterialID = materialName;
    material = nullptr;
}

std::string MeshImporter::GetTextureTypeName(aiTextureType type)
{
    switch (type) {
    case aiTextureType_DIFFUSE:
    case aiTextureType_BASE_COLOR:     return "Diffuse"; // Unifica BASE_COLOR con Diffuse
    case aiTextureType_SPECULAR:       return "Specular";
    case aiTextureType_AMBIENT:        return "Ambient";
    case aiTextureType_EMISSIVE:
    case aiTextureType_EMISSION_COLOR: return "Emissive"; // Unifica EMISSION_COLOR con Emissive
    case aiTextureType_HEIGHT:         return "Height";
    case aiTextureType_NORMALS:
    case aiTextureType_NORMAL_CAMERA:  return "Normals"; // Unifica NORMAL_CAMERA con Normals
    case aiTextureType_SHININESS:
    case aiTextureType_DIFFUSE_ROUGHNESS: return "Roughness"; // Unifica Roughness y Shininess
    case aiTextureType_OPACITY:        return "Opacity";
    case aiTextureType_DISPLACEMENT:   return "Displacement";
    case aiTextureType_LIGHTMAP:       return "Lightmap";
    case aiTextureType_REFLECTION:     return "Reflection";
    case aiTextureType_METALNESS:      return "Metalness";
    case aiTextureType_AMBIENT_OCCLUSION: return "AO";
    default: return "Unknown";
    }
}

void MeshImporter::SaveTextureToFile(const aiTexture* texture, const std::string& outputPath)
{
    if (filesystem::exists(outputPath))
    {
        return;
    }

    std::ofstream outFile(outputPath, std::ios::binary);
    if (outFile)
    {
        outFile.write(reinterpret_cast<const char*>(texture->pcData), texture->mWidth);
        outFile.close();
        std::cout << "Saved texture: " << outputPath << std::endl;
    }
}

void MeshImporter::CopyTextureFile(const std::string& sourcePath, const std::string& destPath)
{
    if (filesystem::exists(destPath))
    {
        return;
    }

    try
    {
        filesystem::copy(sourcePath, destPath, filesystem::copy_options::overwrite_existing);
        std::cout << "Copied texture: " << destPath << std::endl;
    }
    catch (std::exception& e)
    {
        std::cerr << "Failed to copy texture: " << e.what() << std::endl;
    }
}

void MeshImporter::ExtractAndUpdateTextures(aiScene* scene, const std::string& outputTextureFolder, const std::string& modelDirectory)
{
    std::vector<aiTextureType> textureTypes = {
        aiTextureType_DIFFUSE, aiTextureType_SPECULAR, aiTextureType_NORMALS,
        aiTextureType_EMISSIVE, aiTextureType_HEIGHT, aiTextureType_AMBIENT,
        aiTextureType_SHININESS, aiTextureType_OPACITY, aiTextureType_DISPLACEMENT,
        aiTextureType_LIGHTMAP, aiTextureType_REFLECTION, aiTextureType_BASE_COLOR,
        aiTextureType_METALNESS, aiTextureType_DIFFUSE_ROUGHNESS, aiTextureType_AMBIENT_OCCLUSION
    };

    for (unsigned int i = 0; i < scene->mNumMaterials; i++)
    {
        aiMaterial* material = scene->mMaterials[i];

        for (aiTextureType type : textureTypes)
        {
            aiString texturePath;

            unsigned int index = 0;

            while (material->GetTexture(type, index, &texturePath) == AI_SUCCESS)
            {
                ++index;
                std::string originalFilename = texturePath.C_Str();
                std::string newTexturePath;

                if (originalFilename[0] == '*')
                {  // Textura embebida
                    int textureIndex = std::stoi(originalFilename.substr(1));
                    aiTexture* embeddedTexture = scene->mTextures[textureIndex];

                    std::string extension = (embeddedTexture->mHeight == 0) ? ".jpg" : ".png";
                    std::string filename = std::string(embeddedTexture->mFilename.C_Str());
                    newTexturePath = outputTextureFolder + "/" + filename;

                    if (filename.empty())
                    {
                        std::string textureTypeName = GetTextureTypeName(type);
                        newTexturePath = outputTextureFolder + "/Texture_" + textureTypeName + "_" + std::to_string(i) + extension;
                    }

                    SaveTextureToFile(embeddedTexture, newTexturePath);
                }
                else
                {  // Textura externa
                    std::string sourceTexturePath = modelDirectory + "/" + originalFilename;
                    newTexturePath = outputTextureFolder + "/" + originalFilename;
                    CopyTextureFile(sourceTexturePath, newTexturePath);
                }

                // Actualizamos la referencia en el material
                string parentFolder = filesystem::path(outputTextureFolder).filename().string();
                string finalPath = "../" + parentFolder + "/" + filesystem::path(newTexturePath).filename().string();
                aiString newPath(finalPath);
                material->AddProperty(&newPath, AI_MATKEY_TEXTURE(type, 0));
            }
        }
    }
}

void MeshImporter::ExtractAndUpdateMaterials(aiScene* scene, const std::string& outputTextureFolder, const std::string& outputMaterialPath, const std::string& modelDirectory)
{
    auto matManager = MaterialManager::getInstance();

    std::vector<std::vector<aiTextureType>> textureSlots = {
        { aiTextureType_BASE_COLOR, aiTextureType_DIFFUSE, aiTextureType_AMBIENT }, // Diffuse
        { aiTextureType_NORMALS, aiTextureType_HEIGHT },                            // Normal
        { aiTextureType_SPECULAR, aiTextureType_REFLECTION },                       // Specular
        { aiTextureType_EMISSIVE, aiTextureType_OPACITY },                          // Emissive
        { aiTextureType_HEIGHT, aiTextureType_DISPLACEMENT }                        // Height
    };

    for (unsigned int i = 0; i < scene->mNumMaterials; i++)
    {
        aiMaterial* material = scene->mMaterials[i];

        aiReturn ret;
        aiString rawName;

        ret = material->Get(AI_MATKEY_NAME, rawName);
        if (ret != AI_SUCCESS)
        {
            continue;
        }

        //Check if material name exist & rename it
        std::string materialName = matManager->CheckName(rawName.C_Str());
        aiString newName(materialName);

        // Modify the material name
        material->AddProperty(&newName, AI_MATKEY_NAME);

        std::string filename = materialName + ".qemat";
        fs::path materialPath = fs::path(outputMaterialPath) / filename;

        bool existPath = fs::exists(materialPath);
        if (!existPath)
        {
            MaterialData matData;

            std::ofstream file(materialPath, std::ios::binary);
            if (!file)
            {
                std::cerr << "Error al abrir " << filename <<  " para escritura.\n";
            }

            if (file.is_open())
            {
                // Name
                int materialNameLength = static_cast<int>(materialName.length());
                file.write(reinterpret_cast<const char*>(&materialNameLength), sizeof(int));
                file.write(reinterpret_cast<const char*>(&materialName[0]), materialName.size());

                // Material file path
                std::string materialPathStr = materialPath.string();
                int materialPathLength = static_cast<int>(materialPathStr.length());
                file.write(reinterpret_cast<const char*>(&materialPathLength), sizeof(int));
                file.write(reinterpret_cast<const char*>(&materialPathStr[0]), materialPathLength);

                // Shader
                std::string shaderPath = "default";
                int shaderPathLength = static_cast<int>(shaderPath.length());
                file.write(reinterpret_cast<const char*>(&shaderPathLength), sizeof(int));
                file.write(reinterpret_cast<const char*>(&shaderPath[0]), shaderPath.size());

                // Render layer
                int renderLayer = 1;
                file.write(reinterpret_cast<const char*>(&renderLayer), sizeof(int));

                // Material data
                matData.ImportAssimpMaterial(material);

                file.write(reinterpret_cast<const char*>(&matData.Opacity), sizeof(float));
                file.write(reinterpret_cast<const char*>(&matData.BumpScaling), sizeof(float));
                file.write(reinterpret_cast<const char*>(&matData.Shininess), sizeof(float));
                file.write(reinterpret_cast<const char*>(&matData.Reflectivity), sizeof(float));
                file.write(reinterpret_cast<const char*>(&matData.Shininess_Strength), sizeof(float));
                file.write(reinterpret_cast<const char*>(&matData.Refractivity), sizeof(float));

                file.write(reinterpret_cast<const char*>(&matData.Diffuse), sizeof(glm::vec4));
                file.write(reinterpret_cast<const char*>(&matData.Ambient), sizeof(glm::vec4));
                file.write(reinterpret_cast<const char*>(&matData.Specular), sizeof(glm::vec4));
                file.write(reinterpret_cast<const char*>(&matData.Emissive), sizeof(glm::vec4));
                file.write(reinterpret_cast<const char*>(&matData.Transparent), sizeof(glm::vec4));
                file.write(reinterpret_cast<const char*>(&matData.Reflective), sizeof(glm::vec4));

                // Textures
                std::vector<std::string> finalPaths;
                int validTextureCount = 0;

                for (const auto& fallbackList : textureSlots)
                {
                    aiString texPath;
                    bool found = false;

                    for (aiTextureType type : fallbackList)
                    {
                        if (material->GetTexture(type, 0, &texPath) == AI_SUCCESS)
                        {
                            found = true;
                            break;
                        }
                    }

                    if (found)
                    {
                        finalPaths.push_back(texPath.C_Str());
                        validTextureCount++;
                    }
                    else
                    {
                        finalPaths.push_back(""); // No hay textura
                    }
                }

                // 1. Guardar el número real de texturas encontradas
                //file.write(reinterpret_cast<const char*>(&validTextureCount), sizeof(int));

                // Guardar cada textura (longitud + path si existe)
                for (const std::string& path : finalPaths)
                {
                    int length = static_cast<int>(path.length());
                    file.write(reinterpret_cast<const char*>(&length), sizeof(int));

                    if (length > 0)
                    {
                        file.write(reinterpret_cast<const char*>(&path[0]), length);
                    }
                }

                file.close();
            }
        }
    }
}

void MeshImporter::RemoveOnlyEmbeddedTextures(aiScene* scene)
{
    if (!scene->mTextures || scene->mNumTextures == 0) return;

    std::vector<aiTexture*> texturesToKeep;

    for (unsigned int i = 0; i < scene->mNumTextures; i++) {
        aiTexture* texture = scene->mTextures[i];

        // Las texturas embebidas tienen nombres que empiezan con '*'
        if (texture->mFilename.length > 0 && texture->mFilename.C_Str()[0] != '*') {
            texturesToKeep.push_back(texture); // Conservar las externas
        }
        else {
            delete texture; // Eliminar embebida
        }
    }

    // Actualizar la escena con solo las texturas externas
    if (!texturesToKeep.empty()) {
        scene->mNumTextures = static_cast<unsigned int>(texturesToKeep.size());
        scene->mTextures = new aiTexture * [scene->mNumTextures];

        for (size_t i = 0; i < texturesToKeep.size(); i++)
        {
            scene->mTextures[i] = texturesToKeep[i];
        }
    }
    else {
        scene->mNumTextures = 0;
        scene->mTextures = nullptr;
    }
}

bool MeshImporter::LoadAndExportModel(
    const std::string& inputPath,
    const std::string& outputMeshPath,
    const std::string& outputMaterialPath,
    const std::string& outputTexturePath,
    const std::string& outputAnimationPath)
{
    unsigned int flags = aiProcess_EmbedTextures | aiProcess_Triangulate | aiProcess_GenNormals;
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(inputPath, flags);

    if (!scene) {
        std::cerr << "Error al cargar el modelo: " << importer.GetErrorString() << std::endl;
        return false;
    }

    // Exportar animaciones, si tiene...
    AnimationImporter::ImportAnimation(scene, outputAnimationPath);

    // Comprobar si tiene materiales y texturas
    if (scene->HasMaterials())
    {
        std::string modelDirectory = filesystem::path(inputPath).parent_path().string();

        ExtractAndUpdateTextures(const_cast<aiScene*>(scene), outputTexturePath, modelDirectory);

        ExtractAndUpdateMaterials(const_cast<aiScene*>(scene), outputTexturePath, outputMaterialPath, modelDirectory);
    }

    // Exportar a glTF 2.0
    Assimp::Exporter exporter;

    aiScene* editableScene = new aiScene();
    aiCopyScene(scene, &editableScene);
    RemoveOnlyEmbeddedTextures(editableScene);

    SanitizerHelper::SanitizeSceneNames(editableScene);

    if (exporter.Export(editableScene, "gltf2", outputMeshPath) != AI_SUCCESS)
    {
        std::cerr << "Error al exportar a glTF: " << exporter.GetErrorString() << std::endl;
        AnimationImporter::DestroyScene(editableScene);
        return false;
    }

    AnimationImporter::DestroyScene(editableScene);
    std::cout << "Exportación exitosa: " << outputMeshPath << std::endl;
    return true;
}
