#include "MeshImporter.h"

MeshImporter::MeshImporter()
{
    this->materialManager = MaterialManager::getInstance();
    this->textureManager = TextureManager::getInstance();
}

void MeshImporter::CheckPaths(std::string path)
{
    this->fileExtension = path.substr(path.size() - 3, path.size());

    this->meshPath = path.substr(0, path.find_last_of('/'));

    if (this->fileExtension == "fbx")
    {
        std::size_t pos = path.find("/source");
        this->texturePath = path.substr(0, pos);
        this->texturePath += "/textures/";
    }
    else
    {
        this->texturePath = this->meshPath + "/";
    }
}

void MeshImporter::RecreateNormals(std::vector<PBRVertex>& vertices, std::vector<unsigned int>& indices)
{
    for (size_t v = 0; v < vertices.size(); v++)
    {
        vertices.at(v).norm = glm::vec3(0.0f);
    }

    for (size_t idTr = 0; idTr < indices.size(); idTr += 3)
    {
        size_t idx0 = indices[idTr];
        size_t idx1 = indices[idTr + 1];
        size_t idx2 = indices[idTr + 2];
        glm::vec3 edge1 = vertices[idx1].pos - vertices[idx0].pos;
        glm::vec3 edge2 = vertices[idx2].pos - vertices[idx0].pos;
        glm::vec3 crossProduct = glm::cross(edge1, edge2);

        vertices[idx0].norm += crossProduct;
        vertices[idx1].norm += crossProduct;
        vertices[idx2].norm += crossProduct;
    }

    for (size_t v = 0; v < vertices.size(); v++)
    {
        vertices.at(v).norm = glm::normalize(vertices.at(v).norm);
    }
}

void MeshImporter::SetVertexBoneDataToDefault(PBRVertex& vertex)
{
    for (int i = 0; i < 4; i++)
    {
        vertex.boneIDs[i] = -1;
        vertex.boneWeights[i] = 0.0f;
    }
}

void MeshImporter::SetVertexBoneData(PBRVertex& vertex, int boneID, float weight)
{
    for (int i = 0; i < 4; ++i)
    {
        if (vertex.boneIDs[i] < 0)
        {
            vertex.boneWeights[i] = weight;
            vertex.boneIDs[i] = boneID;
            break;
        }
    }
}

void MeshImporter::ExtractBoneWeightForVertices(MeshData& data, aiMesh* mesh, const aiScene* scene)
{
    for (int boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex)
    {
        int boneID = -1;
        std::string boneName = mesh->mBones[boneIndex]->mName.C_Str();
        if (this->m_BoneInfoMap.find(boneName) == this->m_BoneInfoMap.end())
        {
            BoneInfo newBoneInfo;
            newBoneInfo.id = this->numBones;
            newBoneInfo.offset = ConvertMatrixToGLMFormat(
                mesh->mBones[boneIndex]->mOffsetMatrix);
            this->m_BoneInfoMap[boneName] = newBoneInfo;
            boneID = this->numBones;
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
            for (int weightIndex = 0; weightIndex < numWeights; ++weightIndex)
            {
                int vertexId = weights[weightIndex].mVertexId;
                float weight = weights[weightIndex].mWeight;
                assert(vertexId <= data.vertices.size());
                SetVertexBoneData(data.vertices[vertexId], boneID, weight);
            }
        }
    }
}

void MeshImporter::RecreateTangents(std::vector<PBRVertex>& vertices, std::vector<unsigned int>& indices)
{
    for (size_t idTr = 0; idTr < indices.size(); idTr += 3)
    {
        size_t idx0 = indices[idTr];
        size_t idx1 = indices[idTr + 1];
        size_t idx2 = indices[idTr + 2];
        glm::vec3 edge1 = vertices[idx1].pos - vertices[idx0].pos;
        glm::vec3 edge2 = vertices[idx2].pos - vertices[idx0].pos;
        glm::vec2 deltaUV1 = vertices[idx1].texCoord - vertices[idx0].texCoord;
        glm::vec2 deltaUV2 = vertices[idx2].texCoord - vertices[idx0].texCoord;

        float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

        glm::vec3 tangent, bitangent;

        tangent.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
        tangent.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
        tangent.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
        tangent = glm::normalize(tangent);

        vertices[idx0].Tangents = tangent;
        vertices[idx1].Tangents = tangent;
        vertices[idx2].Tangents = tangent;

        bitangent.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
        bitangent.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
        bitangent.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);
        bitangent = glm::normalize(bitangent);

        vertices[idx0].Bitangents = bitangent;
        vertices[idx1].Bitangents = bitangent;
        vertices[idx2].Bitangents = bitangent;
    }
}

std::vector<MeshData> MeshImporter::LoadMesh(std::string path)
{
    std::vector<MeshData> meshes;
    Assimp::Importer importer;

    scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        fprintf(stderr, "ERROR::ASSIMP::%s", importer.GetErrorString());
        return meshes;
    }

    this->hasAnimation = scene->HasAnimations() && scene->hasSkeletons();

    if (!this->hasAnimation)
    {
        scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace | aiProcess_PreTransformVertices);
    }

    this->CheckPaths(path);

    glm::mat4 parentTransform = this->GetGLMMatrix(scene->mRootNode->mTransformation);
    ProcessNode(scene->mRootNode, scene, parentTransform, meshes);

    this->currentTextures.clear();

    return meshes;
}

void MeshImporter::ProcessNode(aiNode* node, const aiScene* scene, glm::mat4 parentTransform, std::vector<MeshData>& meshes)
{
    glm::mat4 localTransform = this->GetGLMMatrix(node->mTransformation);
    glm::mat4 currentTransform = glm::identity<glm::mat4>();// localTransform* parentTransform;

    // process all the node's meshes (if any)
    for (unsigned int i = 0; i < node->mNumMeshes; i++)
    {
        MeshData result = this->ProcessMesh(scene->mMeshes[node->mMeshes[i]], scene);
        result.name = scene->mMeshes[node->mMeshes[i]]->mName.C_Str();
        result.model = currentTransform;

        this->ProcessMaterial(scene->mMeshes[node->mMeshes[i]], scene, result);

        meshes.push_back(result);
    }

    // then do the same for each of its children
    for (unsigned int i = 0; i < node->mNumChildren; i++)
    {
        this->ProcessNode(node->mChildren[i], scene, currentTransform, meshes);
    }
}

MeshData MeshImporter::ProcessMesh(aiMesh* mesh, const aiScene* scene)
{
    MeshData data = {};

    unsigned int WEIGHTS_PER_VERTEX = 4;
    bool existTangent = mesh->HasTangentsAndBitangents();
    bool existNormal = mesh->HasNormals();

    std::vector<CustomTexture> textures;

    data.numPositions = mesh->mNumVertices;
    data.numVertices = mesh->mNumVertices * 3;
    data.numFaces = mesh->mNumFaces;
    data.numIndices = mesh->mNumFaces * 3;
    data.vertices.resize(data.numVertices);

    for (unsigned int i = 0; i < data.numPositions; i++)
    {
        PBRVertex vertex;

        this->SetVertexBoneDataToDefault(vertex);

        // process vertex positions, normals and texture coordinates
        glm::vec3 vector;
        vector.x = mesh->mVertices[i].x;
        vector.y = mesh->mVertices[i].y;
        vector.z = mesh->mVertices[i].z;
        vertex.pos = vector;

        if (existNormal)
        {
            vector.x = mesh->mNormals[i].x;
            vector.y = mesh->mNormals[i].y;
            vector.z = mesh->mNormals[i].z;
            vertex.norm = vector;
        }

        if (existTangent)
        {
            vector.x = mesh->mTangents[i].x;
            vector.y = mesh->mTangents[i].y;
            vector.z = mesh->mTangents[i].z;
            vertex.Tangents = vector;

            vector.x = mesh->mBitangents[i].x;
            vector.y = mesh->mBitangents[i].y;
            vector.z = mesh->mBitangents[i].z;
            vertex.Bitangents = vector;
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
    }

    // process indices
    data.numIndices = 0;
    for (unsigned int i = 0; i < mesh->mNumFaces; i++)
    {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++)
        {
            data.numIndices += face.mNumIndices;
            data.indices.push_back(face.mIndices[j]);
        }
    }
    data.indices.resize(data.numIndices);


    if (!existNormal)
    {
        this->RecreateNormals(data.vertices, data.indices);
    }

    if (!existTangent)
    {
        this->RecreateTangents(data.vertices, data.indices);
    }

    //ExtractBoneWeightForVertices(data, mesh, scene);

    return data;
}
MeshData MeshImporter::LoadRawMesh(float rawData[], unsigned int numData, unsigned int offset)
{
    MeshData data = {};
    unsigned int NUMCOMP = offset;
    for (int i = 0; i < numData; i++)
    {
        unsigned int index = i * NUMCOMP;

        PBRVertex vertex;
        // process vertex positions, normals and texture coordinates
        glm::vec3 vector;
        vector.x = rawData[index];
        vector.y = rawData[index + 1];
        vector.z = rawData[index + 2];
        vertex.pos = vector;

        vector.x = rawData[index + 3];
        vector.y = rawData[index + 4];
        vector.z = rawData[index + 5];
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

void MeshImporter::ProcessMaterial(aiMesh* mesh, const aiScene* scene, MeshData& meshData)
{
    if (mesh->mMaterialIndex < 0)
        return;

    aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

    if (material == nullptr)
        return;

    auto countTextures = material->GetTextureCount(aiTextureType_NONE);

    aiReturn ret;//Code which says whether loading something has been successful of not

    aiString rawName;
    ret = material->Get(AI_MATKEY_NAME, rawName);//Get the material name (pass by reference)
    if (ret != AI_SUCCESS) rawName = "";//Failed to find material name so makes var empty

    std::string materialName = rawName.C_Str();

    if (!materialManager->Exists(materialName))
    {
        materialManager->CreateMaterial(materialName);
        std::shared_ptr<Material> mat = materialManager->GetMaterial(materialName);

        std::string textureName = this->GetTexture(material, aiTextureType_DIFFUSE, TEXTURE_TYPE::DIFFUSE_TYPE);
        if (textureName != "")
        {
            mat->AddTexture(this->textureManager->GetTexture(textureName));
        }
        else
        {
            textureName = this->GetTexture(material, aiTextureType_BASE_COLOR, TEXTURE_TYPE::DIFFUSE_TYPE);
            if (textureName != "")
            {
                mat->AddTexture(this->textureManager->GetTexture(textureName));
            }
            else
            {
                textureName = this->GetTexture(material, aiTextureType_DIFFUSE_ROUGHNESS, TEXTURE_TYPE::DIFFUSE_TYPE);
                if (textureName != "")
                {
                    mat->AddTexture(this->textureManager->GetTexture(textureName));
                }
            }
        }

        textureName = this->GetTexture(material, aiTextureType_SPECULAR, TEXTURE_TYPE::SPECULAR_TYPE);
        if (textureName != "")
        {
            mat->AddTexture(this->textureManager->GetTexture(textureName));
        }

        textureName = this->GetTexture(material, aiTextureType_NORMALS, TEXTURE_TYPE::NORMAL_TYPE);
        if (textureName != "")
        {
            mat->AddTexture(this->textureManager->GetTexture(textureName));
        }
        else
        {
            textureName = this->GetTexture(material, aiTextureType_HEIGHT, TEXTURE_TYPE::NORMAL_TYPE);
            if (textureName != "")
            {
                mat->AddTexture(this->textureManager->GetTexture(textureName));
            }
        }

        textureName = this->GetTexture(material, aiTextureType_EMISSIVE, TEXTURE_TYPE::EMISSIVE_TYPE);
        if (textureName != "")
        {
            mat->AddTexture(this->textureManager->GetTexture(textureName));
        }

        //textureName = this->GetTexture(material, aiTextureType_HEIGHT, TEXTURE_TYPE::HEIGHT_TYPE);
        //if (textureName != "")
        //{
        //    mat->AddTexture(this->textureManager->GetTexture(textureName));
        //}
    }

    meshData.materialID = materialName;

    material = nullptr;
}

std::string MeshImporter::GetTexture(aiMaterial* mat, aiTextureType type, TEXTURE_TYPE textureType)
{
    aiString str;
    mat->Get(AI_MATKEY_TEXTURE(type, 0), str);

    if (auto texture = scene->GetEmbeddedTexture(str.C_Str())) {
    //    //returned pointer is not null, read texture from memory

        std::string finalName = this->textureManager->AddTexture(texture->mFilename.C_Str(), CustomTexture(texture->pcData, texture->mWidth, texture->mHeight, textureType));
        this->currentTextures.insert(texture->mFilename.C_Str());

        return finalName;
    }
    else {
        aiReturn texFound = AI_SUCCESS;
        texFound = mat->GetTexture(type, 0, &str);

        if (texFound == AI_SUCCESS)
        {
            std::string filePath = std::string(str.C_Str());
            std::string finalName;
            std::size_t pos = 0;
            if (this->fileExtension == "fbx")
            {
                pos = filePath.find("\\");
                finalName = filePath.substr(pos + 1, filePath.size());
            }
            else
            {
                finalName = filePath;
            }

            filePath = this->texturePath + finalName;


            if (this->currentTextures.find(str.C_Str()) == this->currentTextures.end())
            {
                finalName = this->textureManager->AddTexture(finalName, CustomTexture(filePath, textureType));
                this->currentTextures.insert(str.C_Str());
            }

            return finalName;
        }
    }
    return "";
}
