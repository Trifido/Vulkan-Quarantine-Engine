#include "MaterialData.h"

MaterialData::MaterialData()
{
    this->textureManager = TextureManager::getInstance();

    this->emptyTexture = this->textureManager->GetTexture("NULL_TEXTURE");
    this->texture_vector = std::make_shared<std::vector<std::shared_ptr<CustomTexture>>>();
    this->texture_vector->resize(this->TOTAL_NUM_TEXTURES, nullptr);
    this->fillEmptyTextures();
    this->numTextures = 0;
    this->idxDiffuse = this->idxEmissive = this->idxHeight = this->idxNormal = this->idxSpecular = -1;
}

void MaterialData::ImportAssimpMaterial(aiMaterial* material)
{
    aiReturn ret;

    ret = material->Get(AI_MATKEY_OPACITY, this->Opacity);
    if (ret != AI_SUCCESS) this->Opacity = 1.0f;

    ret = material->Get(AI_MATKEY_BUMPSCALING, this->BumpScaling);
    if (ret != AI_SUCCESS) this->BumpScaling = 0.0f;

    ret = material->Get(AI_MATKEY_SHININESS, this->Shininess);
    if (ret != AI_SUCCESS) this->Shininess = 0.0f;

    ret = material->Get(AI_MATKEY_REFLECTIVITY, this->Reflectivity);
    if (ret != AI_SUCCESS) this->Reflectivity = 0.0f;

    ret = material->Get(AI_MATKEY_SHININESS_STRENGTH, this->Shininess_Strength);
    if (ret != AI_SUCCESS) this->Shininess_Strength = 0.0f;

    ret = material->Get(AI_MATKEY_REFRACTI, this->Refractivity);
    if (ret != AI_SUCCESS) this->Refractivity = 0.0f;

    ret = material->Get(AI_MATKEY_COLOR_DIFFUSE, this->Diffuse);
    if (ret != AI_SUCCESS) this->Diffuse = glm::vec3(0.0f);

    ret = material->Get(AI_MATKEY_COLOR_AMBIENT, this->Ambient);
    if (ret != AI_SUCCESS) this->Ambient = glm::vec3(0.0f);

    ret = material->Get(AI_MATKEY_COLOR_SPECULAR, this->Specular);
    if (ret != AI_SUCCESS) this->Specular = glm::vec3(0.0f);

    ret = material->Get(AI_MATKEY_COLOR_EMISSIVE, this->Emissive);
    if (ret != AI_SUCCESS) this->Emissive = glm::vec3(0.0f);

    ret = material->Get(AI_MATKEY_COLOR_TRANSPARENT, this->Transparent);
    if (ret != AI_SUCCESS) this->Transparent = glm::vec3(0.0f);

    ret = material->Get(AI_MATKEY_COLOR_REFLECTIVE, this->Reflective);
    if (ret != AI_SUCCESS) this->Reflective = glm::vec3(0.0f);
}

void MaterialData::ImportAssimpTexture(const aiScene* scene, aiMaterial* material, std::string fileExtension, std::string texturePath)
{
    this->texturePath = texturePath;
    this->fileExtension = fileExtension;

    std::string textureName = this->GetTexture(scene, material, aiTextureType_DIFFUSE, TEXTURE_TYPE::DIFFUSE_TYPE);
    if (textureName != "")
    {
        this->AddTexture(this->textureManager->GetTexture(textureName));
    }
    else
    {
        textureName = this->GetTexture(scene, material, aiTextureType_BASE_COLOR, TEXTURE_TYPE::DIFFUSE_TYPE);
        if (textureName != "")
        {
            this->AddTexture(this->textureManager->GetTexture(textureName));
        }
        else
        {
            textureName = this->GetTexture(scene, material, aiTextureType_DIFFUSE_ROUGHNESS, TEXTURE_TYPE::DIFFUSE_TYPE);
            if (textureName != "")
            {
                this->AddTexture(this->textureManager->GetTexture(textureName));
            }
        }
    }

    textureName = this->GetTexture(scene, material, aiTextureType_SPECULAR, TEXTURE_TYPE::SPECULAR_TYPE);
    if (textureName != "")
    {
        this->AddTexture(this->textureManager->GetTexture(textureName));
    }

    textureName = this->GetTexture(scene, material, aiTextureType_NORMALS, TEXTURE_TYPE::NORMAL_TYPE);
    if (textureName != "")
    {
        this->AddTexture(this->textureManager->GetTexture(textureName));
    }
    else
    {
        textureName = this->GetTexture(scene, material, aiTextureType_HEIGHT, TEXTURE_TYPE::NORMAL_TYPE);
        if (textureName != "")
        {
            this->AddTexture(this->textureManager->GetTexture(textureName));
        }
    }

    textureName = this->GetTexture(scene, material, aiTextureType_EMISSIVE, TEXTURE_TYPE::EMISSIVE_TYPE);
    if (textureName != "")
    {
        this->AddTexture(this->textureManager->GetTexture(textureName));
    }

    textureName = this->GetTexture(scene, material, aiTextureType_HEIGHT, TEXTURE_TYPE::HEIGHT_TYPE);
    if (textureName != "")
    {
        this->AddTexture(this->textureManager->GetTexture(textureName));
    }
}

std::string MaterialData::GetTexture(const aiScene* scene, aiMaterial* mat, aiTextureType type, TEXTURE_TYPE textureType)
{
    aiString str;
    mat->Get(AI_MATKEY_TEXTURE(type, 0), str);

    if (auto texture = scene->GetEmbeddedTexture(str.C_Str()))
    {
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

void MaterialData::AddTexture(std::shared_ptr<CustomTexture> texture)
{
    bool isInserted = false;

    std::shared_ptr<CustomTexture> ptrTexture = this->findTextureByType(texture->type);
    if (ptrTexture == nullptr)
    {
        texture_vector->at(this->numTextures) = texture;
        isInserted = true;
    }
    else
    {
        ptrTexture = texture;
    }

    switch (texture->type)
    {
    case TEXTURE_TYPE::DIFFUSE_TYPE:
    default:
        diffuseTexture = texture;
        if (isInserted) this->idxDiffuse = this->numTextures++;
        break;
    case TEXTURE_TYPE::NORMAL_TYPE:
        normalTexture = texture;
        if (isInserted) this->idxNormal = this->numTextures++;
        break;
    case TEXTURE_TYPE::SPECULAR_TYPE:
        specularTexture = texture;
        if (isInserted) this->idxSpecular = this->numTextures++;
        break;
    case TEXTURE_TYPE::HEIGHT_TYPE:
        heightTexture = texture;
        if (isInserted) this->idxHeight = this->numTextures++;
        break;
    case TEXTURE_TYPE::EMISSIVE_TYPE:
        emissiveTexture = texture;
        if (isInserted) this->idxEmissive = this->numTextures++;
        break;
    }
}

std::shared_ptr<CustomTexture> MaterialData::findTextureByType(TEXTURE_TYPE newtype)
{
    for (size_t id = 0; id < this->TOTAL_NUM_TEXTURES; id++)
    {
        if (this->texture_vector->at(id) != nullptr)
        {
            if (this->texture_vector->at(id)->type == newtype)
                return this->texture_vector->at(id);
        }
        else
        {
            return nullptr;
        }
    }
    return nullptr;
}

void MaterialData::fillEmptyTextures()
{
    for (size_t i = 0; i < this->TOTAL_NUM_TEXTURES; i++)
    {
        if (this->texture_vector->at(i) == nullptr)
        {
            this->texture_vector->at(i) = this->emptyTexture;
        }
    }
}
