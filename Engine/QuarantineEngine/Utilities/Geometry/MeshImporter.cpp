#include "MeshImporter.h"
#include <fstream>
#include <assimp/Exporter.hpp>
#include <assimp/postprocess.h>
#include <assimp/config.h>
#include <unordered_set>

MeshImporter::MeshImporter()
{
    this->materialManager = MaterialManager::getInstance();
    this->shaderManager = ShaderManager::getInstance();
}

void MeshImporter::CheckPaths(std::string path)
{
    fs::path filePath = filesystem::path(path);
    this->fileExtension = filePath.extension().string();

    this->texturePath = filePath.parent_path().parent_path().string() + "/textures/";
}

void MeshImporter::RecreateNormals(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices)
{
    for (size_t v = 0; v < vertices.size(); v++)
    {
        vertices.at(v).norm = glm::vec4(0.0f);
    }

    for (size_t idTr = 0; idTr < indices.size(); idTr += 3)
    {
        size_t idx0 = indices[idTr];
        size_t idx1 = indices[idTr + 1];
        size_t idx2 = indices[idTr + 2];

        glm::vec3 pos0 = glm::vec3(vertices[idx0].pos);
        glm::vec3 pos1 = glm::vec3(vertices[idx1].pos);
        glm::vec3 pos2 = glm::vec3(vertices[idx2].pos);

        glm::vec3 edge1 = pos1 - pos0;
        glm::vec3 edge2 = pos2 - pos0;
        glm::vec3 crossProduct = glm::cross(edge1, edge2);
        glm::vec4 crossProduct4 = glm::vec4(crossProduct.x, crossProduct.y, crossProduct.z, 0.0f);

        vertices[idx0].norm += crossProduct4;
        vertices[idx1].norm += crossProduct4;
        vertices[idx2].norm += crossProduct4;
    }

    for (size_t v = 0; v < vertices.size(); v++)
    {
        vertices.at(v).norm = glm::normalize(vertices.at(v).norm);
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

void MeshImporter::ExtractBoneWeightForVertices(MeshData& data, aiMesh* mesh)
{
    for (unsigned int boneIndex = 0; boneIndex < mesh->mNumBones; boneIndex++)
    {
        int boneID = -1;
        std::string boneName = mesh->mBones[boneIndex]->mName.C_Str();

        if (this->m_BoneInfoMap.find(boneName) == this->m_BoneInfoMap.end())
        {
            BoneInfo newBoneInfo;
            newBoneInfo.id = (int)this->numBones;
            newBoneInfo.offset = ConvertMatrixToGLMFormat(mesh->mBones[boneIndex]->mOffsetMatrix);
            this->m_BoneInfoMap[boneName] = newBoneInfo;
            boneID = (int)this->numBones;
            this->numBones++;
        }
        else
        {
            boneID = this->m_BoneInfoMap[boneName].id;
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

                assert(vertexId <= data.vertices.size());

                SetVertexBoneData(data.animationData[vertexId], boneID, weight);
            }
        }
    }
}

void MeshImporter::RemapGeometry(MeshData& data)
{
    std::vector<unsigned int> remap(data.numIndices);

    std::vector<uint32_t> resultIndices;
    std::vector<Vertex> resultVertices;

    size_t total_vertices = meshopt_generateVertexRemap(&remap[0], &data.indices[0], data.numIndices, &data.vertices[0], data.numIndices, sizeof(Vertex));

    data.numVertices = total_vertices;
    resultIndices.resize(data.numIndices);
    meshopt_remapIndexBuffer(&resultIndices[0], &data.indices[0], data.numIndices, &remap[0]);

    resultVertices.resize(total_vertices);
    meshopt_remapVertexBuffer(&resultVertices[0], &data.vertices[0], data.numIndices, sizeof(Vertex), &remap[0]);

    data.indices = resultIndices;
    data.vertices = resultVertices;
}

void MeshImporter::ComputeAABB(const glm::vec4& coord)
{
    if (this->aabbMin.x > coord.x)
        this->aabbMin.x = coord.x;
    if (this->aabbMin.y > coord.y)
        this->aabbMin.y = coord.y;
    if (this->aabbMin.z > coord.z)
        this->aabbMin.z = coord.z;

    if (this->aabbMax.x < coord.x)
        this->aabbMax.x = coord.x;
    if (this->aabbMax.y < coord.y)
        this->aabbMax.y = coord.y;
    if (this->aabbMax.z < coord.z)
        this->aabbMax.z = coord.z;
}

void MeshImporter::RecreateTangents(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices)
{
    for (size_t idTr = 0; idTr < indices.size(); idTr += 3)
    {
        size_t idx0 = indices[idTr];
        size_t idx1 = indices[idTr + 1];
        size_t idx2 = indices[idTr + 2];


        glm::vec3 pos0 = glm::vec3(vertices[idx0].pos);
        glm::vec3 pos1 = glm::vec3(vertices[idx1].pos);
        glm::vec3 pos2 = glm::vec3(vertices[idx2].pos);


        glm::vec3 edge1 = pos1 - pos0;
        glm::vec3 edge2 = pos2 - pos0;
        glm::vec2 deltaUV1 = vertices[idx1].texCoord - vertices[idx0].texCoord;
        glm::vec2 deltaUV2 = vertices[idx2].texCoord - vertices[idx0].texCoord;

        float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

        glm::vec4 tangent;

        tangent.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
        tangent.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
        tangent.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
        tangent.w = 0.0f;
        tangent = glm::normalize(tangent);

        vertices[idx0].Tangents = tangent;
        vertices[idx1].Tangents = tangent;
        vertices[idx2].Tangents = tangent;
    }
}

std::vector<MeshData> MeshImporter::LoadMesh(std::string path)
{
    fs::path filepath = fs::path(path);
    fs::path matpath = filepath.parent_path().parent_path() / "Materials";
    std::vector<MeshData> meshes;
    Assimp::Importer importer;
    (void)importer.SetPropertyInteger(AI_CONFIG_PP_LBW_MAX_WEIGHTS, 4);
    scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace | aiProcess_LimitBoneWeights);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        fprintf(stderr, "ERROR::ASSIMP::%s", importer.GetErrorString());
        return meshes;
    }

    this->hasAnimation = scene->HasAnimations();

    if (!this->hasAnimation)
    {
        scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace | aiProcess_PreTransformVertices | aiProcess_PopulateArmatureData);
    }

    this->CheckPaths(path);

    this->aabbMax = glm::vec3(-std::numeric_limits<float>::infinity());
    this->aabbMin = glm::vec3(std::numeric_limits<float>::infinity());

    glm::mat4 parentTransform = glm::mat4(1.0f);
    ProcessNode(scene->mRootNode, scene, parentTransform, meshes, matpath);

    return meshes;
}

void MeshImporter::ProcessNode(aiNode* node, const aiScene* scene, glm::mat4 parentTransform, std::vector<MeshData>& meshes, const fs::path& matpath)
{
    glm::mat4 localTransform = this->GetGLMMatrix(node->mTransformation);
    glm::mat4 currentTransform = glm::identity<glm::mat4>();

    // process all the node's meshes (if any)
    for (unsigned int i = 0; i < node->mNumMeshes; i++)
    {
        MeshData result = this->ProcessMesh(scene->mMeshes[node->mMeshes[i]], scene);
        result.name = scene->mMeshes[node->mMeshes[i]]->mName.C_Str();
        result.model = currentTransform;

        this->ProcessMaterial(scene->mMeshes[node->mMeshes[i]], scene, result, matpath);

        meshes.push_back(result);
    }

    // then do the same for each of its children
    for (unsigned int i = 0; i < node->mNumChildren; i++)
    {
        this->ProcessNode(node->mChildren[i], scene, currentTransform, meshes, matpath);
    }
}

MeshData MeshImporter::ProcessMesh(aiMesh* mesh, const aiScene* scene)
{
    MeshData data = {};

    bool existTangent = mesh->HasTangentsAndBitangents();
    bool existNormal = mesh->HasNormals();

    std::vector<CustomTexture> textures;

    data.numVertices = mesh->mNumVertices;
    data.numFaces = mesh->mNumFaces;
    data.numIndices = mesh->mNumFaces * 3;

    data.vertices.resize(data.numVertices);
    data.animationData.resize(data.numVertices);
    for (unsigned int i = 0; i < data.numVertices; i++)
    {
        Vertex vertex;
        AnimationVertexData animData;

        this->SetVertexBoneDataToDefault(animData);

        // process vertex positions, normals and texture coordinates
        glm::vec4 vector;
        vector.x = mesh->mVertices[i].x;
        vector.y = mesh->mVertices[i].y;
        vector.z = mesh->mVertices[i].z;
        vector.w = 1.0f;
        vertex.pos = vector;

        this->ComputeAABB(vertex.pos);

        if (existNormal)
        {
            vector.x = mesh->mNormals[i].x;
            vector.y = mesh->mNormals[i].y;
            vector.z = mesh->mNormals[i].z;
            vector.w = 0.0f;
            vertex.norm = vector;
        }

        if (existTangent)
        {
            vector.x = mesh->mTangents[i].x;
            vector.y = mesh->mTangents[i].y;
            vector.z = mesh->mTangents[i].z;
            vector.w = 0.0f;
            vertex.Tangents = vector;
        }

        if (mesh->mTextureCoords[0]) // does the mesh contain texture coordinates?
        {
            glm::vec2 vec;
            vec.x = mesh->mTextureCoords[0][i].x;
            vec.y = mesh->mTextureCoords[0][i].y;
            vertex.texCoord = vec;
        }
        else
        {
            vertex.texCoord = glm::vec2(0.0f, 0.0f);
        }

        data.vertices.at(i) = vertex;
        data.animationData.at(i) = animData;
    }

    // process indices
    data.numIndices = mesh->mNumFaces * mesh->mFaces[0].mNumIndices;
    data.indices.reserve(data.numIndices);

    for (unsigned int i = 0; i < mesh->mNumFaces; i++)
    {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++)
        {
            data.indices.push_back(face.mIndices[j]);
        }
    }

    if (!existNormal)
    {
        this->RecreateNormals(data.vertices, data.indices);
    }

    if (!existTangent)
    {
        this->RecreateTangents(data.vertices, data.indices);
    }

    ExtractBoneWeightForVertices(data, mesh);

    this->RemapGeometry(data);

    data.HasAnimation = this->hasAnimation;

    return data;
}

MeshData MeshImporter::LoadRawMesh(float rawData[], unsigned int numData, unsigned int offset)
{
    MeshData data = {};
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
        vertex.pos = vector;

        vector.x = rawData[index + 3];
        vector.y = rawData[index + 4];
        vector.z = rawData[index + 5];
        vector.w = 0.0f;
        vertex.norm = vector;

        glm::vec2 vec;
        vec.x = rawData[index + 6];
        vec.y = rawData[index + 7];
        vertex.texCoord = vec;

        data.vertices.push_back(vertex);
        data.indices.push_back(i);
    }

    RecreateTangents(data.vertices, data.indices);

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

void MeshImporter::ProcessMaterial(aiMesh* mesh, const aiScene* scene, MeshData& meshData, const fs::path& matpath)
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
    std::shared_ptr<Material> mat_ptr;
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

            auto shader = shaderManager->GetShader(matDto.ShaderPath);
            if (shader == nullptr)
            {
                std::cerr << "Error: Shader not found for material " << materialName << std::endl;
                return;
            }

            matDto.UpdateTexturePaths(matpath);

            mat_ptr = std::make_shared<Material>(Material(shader, matDto));
            materialManager->AddMaterial(mat_ptr);
        }
    }
    else
    {
        mat_ptr = materialManager->GetMaterial(materialName);
    }

    mat_ptr->InitializeMaterialDataUBO();

    meshData.materialID = materialName;

    material = nullptr;
}

std::pair<glm::vec3, glm::vec3> MeshImporter::GetAABBData()
{
    return std::pair<glm::vec3, glm::vec3>(this->aabbMin, this->aabbMax);
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

        for (size_t i = 0; i < texturesToKeep.size(); i++) {
            scene->mTextures[i] = texturesToKeep[i];
        }
    }
    else {
        scene->mNumTextures = 0;
        scene->mTextures = nullptr;
    }
}

bool MeshImporter::LoadAndExportModel(const std::string& inputPath, const std::string& outputMeshPath, const std::string& outputMaterialPath, const std::string& outputTexturePath)
{
    unsigned int flags = aiProcess_EmbedTextures | aiProcess_Triangulate | aiProcess_GenNormals;
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(inputPath, flags);

    if (!scene) {
        std::cerr << "Error al cargar el modelo: " << importer.GetErrorString() << std::endl;
        return false;
    }

    // Comprobar si tiene materiales y texturas
    if (scene->HasMaterials())
    {
        std::cout << "El modelo tiene " << scene->mNumMaterials << " materiales." << std::endl;

        std::string modelDirectory = filesystem::path(inputPath).parent_path().string();

        ExtractAndUpdateTextures(const_cast<aiScene*>(scene), outputTexturePath, modelDirectory);

        ExtractAndUpdateMaterials(const_cast<aiScene*>(scene), outputTexturePath, outputMaterialPath, modelDirectory);
    }

    // Comprobar si tiene animaciones
    if (scene->HasAnimations()) {
        std::cout << "El modelo tiene " << scene->mNumAnimations << " animaciones." << std::endl;
    }

    // Exportar a glTF 2.0
    Assimp::Exporter exporter;

    aiScene* editableScene = new aiScene();
    aiCopyScene(scene, &editableScene);
    RemoveOnlyEmbeddedTextures(editableScene);

    if (exporter.Export(editableScene, "gltf2", outputMeshPath) != AI_SUCCESS)
    {
        std::cerr << "Error al exportar a glTF: " << exporter.GetErrorString() << std::endl;
        return false;
    }

    std::cout << "Exportación exitosa: " << outputMeshPath << std::endl;
    return true;
}
