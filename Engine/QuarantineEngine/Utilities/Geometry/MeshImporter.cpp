#include "MeshImporter.h"


MeshImporter::MeshImporter()
{

}

MeshImporter::MeshImporter(std::string pathfile)
{
}

void MeshImporter::LoadMesh(std::string path)
{
    Assimp::Importer importer;
    scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        fprintf(stderr, "ERROR::ASSIMP::%s", importer.GetErrorString());
        return;
    }

    this->ProcessNode(scene->mRootNode, scene);
}

void MeshImporter::ProcessNode(aiNode* node, const aiScene* scene)
{
    // process all the node's meshes (if any)
    for (unsigned int i = 0; i < node->mNumMeshes; i++)
    {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        meshes.push_back(this->ProcessMesh(mesh, scene));
    }
    // then do the same for each of its children
    for (unsigned int i = 0; i < node->mNumChildren; i++)
    {
        this->ProcessNode(node->mChildren[i], scene);
    }
}

Mesh MeshImporter::ProcessMesh(aiMesh* mesh, const aiScene* scene)
{
    unsigned int WEIGHTS_PER_VERTEX = 4;
    existTangent = mesh->HasTangentsAndBitangents();
    existNormal = mesh->HasNormals();
    std::vector<PBRVertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<CustomTexture> textures;

    numVertices = mesh->mNumVertices * 3;
    numFaces = mesh->mNumFaces;

    for (unsigned int i = 0; i < mesh->mNumVertices; i++)
    {
        PBRVertex vertex;
        // process vertex positions, normals and texture coordinates
        glm::vec3 vector;
        vector.x = mesh->mVertices[i].x;
        vector.y = mesh->mVertices[i].y;
        vector.z = mesh->mVertices[i].z;
        vertex.pos = vector;

        rawVertices.push_back(rp3d::Vector3(vector.x, vector.y, vector.z));

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

        vertices.push_back(vertex);
    }
    // process indices
    for (unsigned int i = 0; i < mesh->mNumFaces; i++)
    {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++)
        {
            indices.push_back(face.mIndices[j]);
        }
    }
    rawIndices = indices;

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

    return Mesh(vertices, indices, textures, matHandle);
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
            filename = directory + '/' + filename;

            CustomTexture texture(filename);
            texture.path = str.C_Str();
            textures.push_back(texture);
            textures_loaded.push_back(texture); // add to loaded textures
        }
    }
    return textures;
}
