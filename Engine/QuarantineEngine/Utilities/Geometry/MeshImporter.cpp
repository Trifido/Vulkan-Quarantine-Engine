#include "MeshImporter.h"


MeshImporter::MeshImporter()
{
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


MeshData MeshImporter::LoadMesh(std::string path)
{
    Assimp::Importer importer;

    const aiScene* scene;
    scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        fprintf(stderr, "ERROR::ASSIMP::%s", importer.GetErrorString());
        return MeshData();
    }

    //this->ProcessNode(scene->mRootNode, scene);
    return this->ProcessMesh(scene->mMeshes[scene->mRootNode->mMeshes[0]], scene);
}

void MeshImporter::ProcessNode(aiNode* node, const aiScene* scene)
{
    // process all the node's meshes (if any)
    for (unsigned int i = 0; i < node->mNumMeshes; i++)
    {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        //meshes.push_back(this->ProcessMesh(mesh, scene));
    }
    // then do the same for each of its children
    for (unsigned int i = 0; i < node->mNumChildren; i++)
    {
        this->ProcessNode(node->mChildren[i], scene);
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

    return data;
}
/*
void MeshImporter::ProcessTextures(aiMesh* mesh, const aiScene* scene)
{
    // process material
    if (mesh->mMaterialIndex >= 0)
    {
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

        std::vector<CustomTexture> diffuseMaps = this->LoadMaterialTextures(material, aiTextureType_DIFFUSE);
        textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());

        std::vector<CustomTexture> specularMaps = this->LoadMaterialTextures(material, aiTextureType_SPECULAR);
        textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
        // 3. normal maps
        std::vector<CustomTexture> normalMaps = this->LoadMaterialTextures(material, aiTextureType_NORMALS);
        textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
        // 4. height maps
        std::vector<CustomTexture> heightMaps = this->LoadMaterialTextures(material, aiTextureType_AMBIENT);
        textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());
        // metallic maps
        std::vector<CustomTexture> metallicMaps = this->LoadMaterialTextures(material, aiTextureType_METALNESS);
        textures.insert(textures.end(), metallicMaps.begin(), metallicMaps.end());
        // AO maps
        std::vector<CustomTexture> aoMaps = this->LoadMaterialTextures(material, aiTextureType_AMBIENT_OCCLUSION);
        textures.insert(textures.end(), aoMaps.begin(), aoMaps.end());
        // Roughness maps
        std::vector<CustomTexture> roughMaps = this->LoadMaterialTextures(material, aiTextureType_DIFFUSE_ROUGHNESS);
        textures.insert(textures.end(), roughMaps.begin(), roughMaps.end());
        // Bump maps
        std::vector<CustomTexture> bumpMaps;
        if (normalMaps.empty())
            bumpMaps = this->LoadMaterialTextures(material, aiTextureType_HEIGHT);
        else
            bumpMaps = this->LoadMaterialTextures(material, aiTextureType_HEIGHT);

        textures.insert(textures.end(), bumpMaps.begin(), bumpMaps.end());
    }
}

std::vector<CustomTexture> MeshImporter::LoadMaterialTextures(aiMaterial* mat, aiTextureType type)
{
    std::vector<CustomTexture> textures;
    for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
    {
        aiString str;
        mat->GetTexture(type, i, &str);
        bool skip = false;
        for (unsigned int j = 0; j < textures_loaded.size(); j++)
        {
            if (std::strcmp(textures_loaded[j].path.data(), str.C_Str()) == 0)
            {
                textures.push_back(textures_loaded[j]);
                skip = true;
                break;
            }
        }
        if (!skip)
        {   // if texture hasn't been loaded already, load it
            std::string filename = std::string(str.C_Str());
            //filename = directory + '/' + filename;

            CustomTexture texture();
            textures.push_back(texture);
            textures_loaded.push_back(texture); // add to loaded textures
        }
    }
    return textures;
}*/
