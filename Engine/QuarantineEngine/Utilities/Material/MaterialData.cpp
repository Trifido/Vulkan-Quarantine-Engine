#include "MaterialData.h"
#include <SynchronizationModule.h>

MaterialData::MaterialData()
{
    this->deviceModule = DeviceModule::getInstance();
    this->textureManager = TextureManager::getInstance();

    this->emptyTexture = this->textureManager->GetTexture("NULL_TEXTURE");
    this->texture_vector = std::make_shared<std::vector<std::shared_ptr<CustomTexture>>>();
    this->texture_vector->resize(this->TOTAL_NUM_TEXTURES, nullptr);
    this->fillEmptyTextures();
    this->numTextures = 0;
    this->idxDiffuse = this->idxEmissive = this->idxHeight = this->idxNormal = this->idxSpecular = -1;

    this->materialUBO = std::make_shared<UniformBufferObject>();
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

void MaterialData::InitializeUBOMaterial(std::shared_ptr<ShaderModule> shader_ptr)
{
    auto reflect = shader_ptr->reflectShader;

    if (!reflect.materialUBOComponents.empty())
    {
        size_t position = 0;
        this->materialbuffer = new char[reflect.materialBufferSize];

        uint16_t count = 0;
        for each (auto name in reflect.materialUBOComponents)
        {
            if (name == "Opacity")
            {
                this->WriteToMaterialBuffer(reinterpret_cast<char*>(&this->Opacity), position, sizeof(this->Opacity));
                continue;
            }
            if (name == "BumpScaling")
            {
                this->WriteToMaterialBuffer(reinterpret_cast<char*>(&this->BumpScaling), position, sizeof(this->BumpScaling));
                continue;
            }
            if (name == "Shininess")
            {
                this->WriteToMaterialBuffer(reinterpret_cast<char*>(&this->Shininess), position, sizeof(this->Shininess));
                continue;
            }
            if (name == "Reflectivity")
            {
                this->WriteToMaterialBuffer(reinterpret_cast<char*>(&this->Reflectivity), position, sizeof(this->Reflectivity));
                continue;
            }
            if (name == "Shininess_Strength")
            {
                this->WriteToMaterialBuffer(reinterpret_cast<char*>(&this->Shininess_Strength), position, sizeof(this->Shininess_Strength));
                continue;
            }
            if (name == "Refractivity")
            {
                this->WriteToMaterialBuffer(reinterpret_cast<char*>(&this->Refractivity), position, sizeof(this->Refractivity));
                continue;
            }

            if (name == "idxDiffuse")
            {
                this->WriteToMaterialBuffer(reinterpret_cast<char*>(&this->idxDiffuse), position, sizeof(this->idxDiffuse));
                continue;
            }
            if (name == "idxNormal")
            {
                this->WriteToMaterialBuffer(reinterpret_cast<char*>(&this->idxNormal), position, sizeof(this->idxNormal));
                continue;
            }
            if (name == "idxSpecular")
            {
                this->WriteToMaterialBuffer(reinterpret_cast<char*>(&this->idxSpecular), position, sizeof(this->idxSpecular));
                continue;
            }
            if (name == "idxHeight")
            {
                this->WriteToMaterialBuffer(reinterpret_cast<char*>(&this->idxHeight), position, sizeof(this->idxHeight));
                continue;
            }
            if (name == "idxEmissive")
            {
                this->WriteToMaterialBuffer(reinterpret_cast<char*>(&this->idxEmissive), position, sizeof(this->idxEmissive));
                continue;
            }

            if (name == "Diffuse")
            {
                this->WriteToMaterialBuffer(reinterpret_cast<char*>(&this->Diffuse), position, sizeof(this->Diffuse));
                continue;
            }
            if (name == "Ambient")
            {
                this->WriteToMaterialBuffer(reinterpret_cast<char*>(&this->Ambient), position, sizeof(this->Ambient));
                continue;
            }
            if (name == "Specular")
            {
                this->WriteToMaterialBuffer(reinterpret_cast<char*>(&this->Specular), position, sizeof(this->Specular));
                continue;
            }
            if (name == "Emissive")
            {
                this->WriteToMaterialBuffer(reinterpret_cast<char*>(&this->Emissive), position, sizeof(this->Emissive));
                continue;
            }
            if (name == "Transparent")
            {
                this->WriteToMaterialBuffer(reinterpret_cast<char*>(&this->Transparent), position, sizeof(this->Transparent));
                continue;
            }
            if (name == "Reflective")
            {
                this->WriteToMaterialBuffer(reinterpret_cast<char*>(&this->Reflective), position, sizeof(this->Reflective));
                continue;
            }
        }

        //Check result
        //MaterialUniform result;
        //size_t resultSize = sizeof(result);
        //memcpy(&result, this->materialbuffer, this->rawSize);

        this->materialUBO->CreateUniformBuffer(this->rawSize, MAX_FRAMES_IN_FLIGHT, *deviceModule);
        this->UpdateUBOMaterial();
    }

    if (reflect.isUboAnimation)
    {
        size_t position = 0;
        this->animationbuffer = new char[reflect.animationBufferSize];
        this->animationUBO = std::make_shared<UniformBufferObject>();
        this->animationUBO->CreateUniformBuffer(reflect.animationBufferSize, MAX_FRAMES_IN_FLIGHT, *deviceModule);

        uint16_t count = 0;
        for each (auto name in reflect.animationUBOComponents)
        {
            if (name == "finalBonesMatrices")
            {
                auto currentFrame = SynchronizationModule::GetCurrentFrame();
                void* data;
                vkMapMemory(deviceModule->device, this->animationUBO->uniformBuffersMemory[currentFrame], 0, reflect.animationBufferSize, 0, &data);
                memcpy(data, this->animationbuffer, reflect.animationBufferSize);
                vkUnmapMemory(deviceModule->device, this->animationUBO->uniformBuffersMemory[currentFrame]);
                continue;
            }
        }
    }
}

void MaterialData::UpdateUBOMaterial()
{
    auto currentFrame = SynchronizationModule::GetCurrentFrame();
    void* data;
    vkMapMemory(deviceModule->device, this->materialUBO->uniformBuffersMemory[currentFrame], 0, this->rawSize, 0, &data);
    memcpy(data, this->materialbuffer, this->rawSize);
    vkUnmapMemory(deviceModule->device, this->materialUBO->uniformBuffersMemory[currentFrame]);
}

void MaterialData::WriteToMaterialBuffer(char* data, size_t& position, const size_t& sizeToCopy)
{
    memcpy(&materialbuffer[position], data, sizeToCopy);
    position += sizeToCopy;
    this->rawSize += sizeToCopy;
}