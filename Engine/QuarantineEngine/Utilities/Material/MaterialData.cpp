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
    this->Shininess = 32.0f;

    this->materialUBO = std::make_shared<UniformBufferObject>();
    this->isModified = true;
}

void MaterialData::ImportAssimpMaterial(aiMaterial* material)
{
    aiReturn ret;

    ret = material->Get(AI_MATKEY_OPACITY, this->Opacity);
    if (ret != AI_SUCCESS) this->Opacity = 1.0f;

    ret = material->Get(AI_MATKEY_BUMPSCALING, this->BumpScaling);
    if (ret != AI_SUCCESS) this->BumpScaling = 0.0f;

    ret = material->Get(AI_MATKEY_SHININESS, this->Shininess);
    if (ret != AI_SUCCESS) this->Shininess = 32.0f;

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

void MaterialData::CleanLastResources()
{
    this->deviceModule = nullptr;
    this->textureManager = nullptr;
    this->currentTextures.clear();
    this->materialbuffer = nullptr;
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
                this->materialFields["Opacity"] = std::pair<size_t, size_t>(position, sizeof(this->Opacity));
                this->WriteToMaterialBuffer(reinterpret_cast<char*>(&this->Opacity), position, sizeof(this->Opacity));
                continue;
            }
            if (name == "BumpScaling")
            {
                this->materialFields["BumpScaling"] = std::pair<size_t, size_t>(position, sizeof(this->BumpScaling));
                this->WriteToMaterialBuffer(reinterpret_cast<char*>(&this->BumpScaling), position, sizeof(this->BumpScaling));
                continue;
            }
            if (name == "Shininess")
            {
                this->materialFields["Shininess"] = std::pair<size_t, size_t>(position, sizeof(this->Shininess));
                this->WriteToMaterialBuffer(reinterpret_cast<char*>(&this->Shininess), position, sizeof(this->Shininess));
                continue;
            }
            if (name == "Reflectivity")
            {
                this->materialFields["Reflectivity"] = std::pair<size_t, size_t>(position, sizeof(this->Reflectivity));
                this->WriteToMaterialBuffer(reinterpret_cast<char*>(&this->Reflectivity), position, sizeof(this->Reflectivity));
                continue;
            }
            if (name == "Shininess_Strength")
            {
                this->materialFields["Shininess_Strength"] = std::pair<size_t, size_t>(position, sizeof(this->Shininess_Strength));
                this->WriteToMaterialBuffer(reinterpret_cast<char*>(&this->Shininess_Strength), position, sizeof(this->Shininess_Strength));
                continue;
            }
            if (name == "Refractivity")
            {
                this->materialFields["Refractivity"] = std::pair<size_t, size_t>(position, sizeof(this->Refractivity));
                this->WriteToMaterialBuffer(reinterpret_cast<char*>(&this->Refractivity), position, sizeof(this->Refractivity));
                continue;
            }

            if (name == "idxDiffuse")
            {
                this->materialFields["idxDiffuse"] = std::pair<size_t, size_t>(position, sizeof(this->idxDiffuse));
                this->WriteToMaterialBuffer(reinterpret_cast<char*>(&this->idxDiffuse), position, sizeof(this->idxDiffuse));
                continue;
            }
            if (name == "idxNormal")
            {
                this->materialFields["idxNormal"] = std::pair<size_t, size_t>(position, sizeof(this->idxNormal));
                this->WriteToMaterialBuffer(reinterpret_cast<char*>(&this->idxNormal), position, sizeof(this->idxNormal));
                continue;
            }
            if (name == "idxSpecular")
            {
                this->materialFields["idxSpecular"] = std::pair<size_t, size_t>(position, sizeof(this->idxSpecular));
                this->WriteToMaterialBuffer(reinterpret_cast<char*>(&this->idxSpecular), position, sizeof(this->idxSpecular));
                continue;
            }
            if (name == "idxHeight")
            {
                this->materialFields["idxHeight"] = std::pair<size_t, size_t>(position, sizeof(this->idxHeight));
                this->WriteToMaterialBuffer(reinterpret_cast<char*>(&this->idxHeight), position, sizeof(this->idxHeight));
                continue;
            }
            if (name == "idxEmissive")
            {
                this->materialFields["idxEmissive"] = std::pair<size_t, size_t>(position, sizeof(this->idxEmissive));
                this->WriteToMaterialBuffer(reinterpret_cast<char*>(&this->idxEmissive), position, sizeof(this->idxEmissive));
                continue;
            }

            if (name == "Diffuse")
            {
                this->materialFields["Diffuse"] = std::pair<size_t, size_t>(position, sizeof(this->Diffuse));
                this->WriteToMaterialBuffer(reinterpret_cast<char*>(&this->Diffuse), position, sizeof(this->Diffuse));
                continue;
            }
            if (name == "Ambient")
            {
                this->materialFields["Ambient"] = std::pair<size_t, size_t>(position, sizeof(this->Ambient));
                this->WriteToMaterialBuffer(reinterpret_cast<char*>(&this->Ambient), position, sizeof(this->Ambient));
                continue;
            }
            if (name == "Specular")
            {
                this->materialFields["Specular"] = std::pair<size_t, size_t>(position, sizeof(this->Specular));
                this->WriteToMaterialBuffer(reinterpret_cast<char*>(&this->Specular), position, sizeof(this->Specular));
                continue;
            }
            if (name == "Emissive")
            {
                this->materialFields["Emissive"] = std::pair<size_t, size_t>(position, sizeof(this->Emissive));
                this->WriteToMaterialBuffer(reinterpret_cast<char*>(&this->Emissive), position, sizeof(this->Emissive));
                continue;
            }
            if (name == "Transparent")
            {
                this->materialFields["Transparent"] = std::pair<size_t, size_t>(position, sizeof(this->Transparent));
                this->WriteToMaterialBuffer(reinterpret_cast<char*>(&this->Transparent), position, sizeof(this->Transparent));
                continue;
            }
            if (name == "Reflective")
            {
                this->materialFields["Reflective"] = std::pair<size_t, size_t>(position, sizeof(this->Reflective));
                this->WriteToMaterialBuffer(reinterpret_cast<char*>(&this->Reflective), position, sizeof(this->Reflective));
                continue;
            }
        }

        //Check result
        //MaterialUniform result;
        //size_t resultSize = sizeof(result);
        //memcpy(&result, this->materialbuffer, this->materialUniformSize);

        this->materialUBO->CreateUniformBuffer(this->materialUniformSize, MAX_FRAMES_IN_FLIGHT, *deviceModule);
        this->UpdateUBOMaterial();
    }
}


void MaterialData::UpdateMaterialData(std::string materialField, char* value)
{
    auto ptr = materialFields.find(materialField);
    if (ptr != materialFields.end())
    {
        this->WriteToMaterialBuffer(value, materialFields[materialField].first, materialFields[materialField].second);
        this->isModified = true;
    }
}

void MaterialData::UpdateUBOMaterial()
{
    if (!this->materialUBO->uniformBuffers.empty()/* && this->isModified*/)
    {
        auto currentFrame = SynchronizationModule::GetCurrentFrame();
        void* data;
        vkMapMemory(deviceModule->device, this->materialUBO->uniformBuffersMemory[currentFrame], 0, this->materialUniformSize, 0, &data);
        memcpy(data, this->materialbuffer, this->materialUniformSize);
        vkUnmapMemory(deviceModule->device, this->materialUBO->uniformBuffersMemory[currentFrame]);

        this->isModified = false;
    }
}

void MaterialData::CleanMaterialUBO()
{
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        // Material UBO
        if (!this->materialUBO->uniformBuffers.empty())
        {
            vkDestroyBuffer(deviceModule->device, this->materialUBO->uniformBuffers[i], nullptr);
            vkFreeMemory(deviceModule->device, this->materialUBO->uniformBuffersMemory[i], nullptr);
        }
    }
}

void MaterialData::WriteToMaterialBuffer(char* data, size_t& position, const size_t& sizeToCopy)
{
    memcpy(&materialbuffer[position], data, sizeToCopy);
    position += sizeToCopy;
    this->materialUniformSize += sizeToCopy;
}

void MaterialData::SetMaterialField(std::string nameField, float value)
{
    if (nameField == "Opacity")
    {
        this->Opacity = value;
        this->UpdateMaterialData(nameField, reinterpret_cast<char*>(&this->Opacity));
    }
    if (nameField == "BumpScaling")
    {
        this->BumpScaling = value;
        this->UpdateMaterialData(nameField, reinterpret_cast<char*>(&this->BumpScaling));
    }
    if (nameField == "Shininess")
    {
        this->Shininess = value;
        this->UpdateMaterialData(nameField, reinterpret_cast<char*>(&this->Shininess));
    }
    if (nameField == "Reflectivity")
    {
        this->Reflectivity = value;
        this->UpdateMaterialData(nameField, reinterpret_cast<char*>(&this->Reflectivity));
    }
    if (nameField == "Shininess_Strength")
    {
        this->Shininess_Strength = value;
        this->UpdateMaterialData(nameField, reinterpret_cast<char*>(&this->Shininess_Strength));
    }
    if (nameField == "Refractivity")
    {
        this->Refractivity = value;
        this->UpdateMaterialData(nameField, reinterpret_cast<char*>(&this->Refractivity));
    }
}

void MaterialData::SetMaterialField(std::string nameField, glm::vec3 value)
{
    if (nameField == "Diffuse")
    {
        this->Diffuse = value;
        this->UpdateMaterialData(nameField, reinterpret_cast<char*>(&this->Diffuse));
    }
    if(nameField == "Ambient")
    {
        this->Ambient = value;
        this->UpdateMaterialData(nameField, reinterpret_cast<char*>(&this->Ambient));
    }
    if(nameField == "Specular")
    {
        this->Specular = value;
        this->UpdateMaterialData(nameField, reinterpret_cast<char*>(&this->Specular));
    }
    if(nameField == "Emissive")
    {
        this->Emissive = value;
        this->UpdateMaterialData(nameField, reinterpret_cast<char*>(&this->Emissive));
    }
    if(nameField == "Transparent")
    {
        this->Transparent = value;
        this->UpdateMaterialData(nameField, reinterpret_cast<char*>(&this->Transparent));
    }
    if(nameField == "Reflective")
    {
        this->Reflective = value;
        this->UpdateMaterialData(nameField, reinterpret_cast<char*>(&this->Reflective));
    }
}

void MaterialData::SetMaterialField(std::string nameField, int value)
{
    if(nameField == "idxDiffuse")
    {
        this->idxDiffuse = value;
        this->UpdateMaterialData(nameField, reinterpret_cast<char*>(&this->idxDiffuse));
    }
    if(nameField == "idxNormal")
    {
        this->idxNormal = value;
        this->UpdateMaterialData(nameField, reinterpret_cast<char*>(&this->idxNormal));
    }
    if(nameField == "idxSpecular")
    {
        this->idxSpecular = value;
        this->UpdateMaterialData(nameField, reinterpret_cast<char*>(&this->idxSpecular));
    }
    if(nameField == "idxHeight")
    {
        this->idxHeight = value;
        this->UpdateMaterialData(nameField, reinterpret_cast<char*>(&this->idxHeight));
    }
    if(nameField == "idxEmissive")
    {
        this->idxEmissive = value;
        this->UpdateMaterialData(nameField, reinterpret_cast<char*>(&this->idxEmissive));
    }
}
