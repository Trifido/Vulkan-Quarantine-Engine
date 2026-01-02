#include "MaterialData.h"

std::string MaterialData::NormalizeMaterialMemberName(std::string name)
{
    auto pos = name.rfind('.');
    if (pos != std::string::npos)
        name = name.substr(pos + 1);

    return name;
}

std::unordered_map<std::string, WriteFn> MaterialData::BuildMaterialWriters()
{
    std::unordered_map<std::string, WriteFn> w;
    w.reserve(32);

    auto writeValue = [&](const std::string& key, auto getter)
        {
            w[key] = [getter](uint8_t* dst, size_t off)
                {
                    auto v = getter();
                    std::memcpy(dst + off, &v, sizeof(v));
                };
        };

    // vec4
    writeValue("Diffuse", [this] { return Diffuse; });
    writeValue("Ambient", [this] { return Ambient; });
    writeValue("Specular", [this] { return Specular; });
    writeValue("Emissive", [this] { return Emissive; });
    writeValue("Transparent", [this] { return Transparent; });
    writeValue("Reflective", [this] { return Reflective; });

    // ints
    writeValue("idxDiffuse", [this] { return IDTextures[TEXTURE_TYPE::DIFFUSE_TYPE]; });
    writeValue("idxNormal", [this] { return IDTextures[TEXTURE_TYPE::NORMAL_TYPE]; });
    writeValue("idxSpecular", [this] { return IDTextures[TEXTURE_TYPE::SPECULAR_TYPE]; });
    writeValue("idxEmissive", [this] { return IDTextures[TEXTURE_TYPE::EMISSIVE_TYPE]; });
    writeValue("idxHeight", [this] { return IDTextures[TEXTURE_TYPE::HEIGHT_TYPE]; });

    // floats
    writeValue("Opacity", [this] { return Opacity; });
    writeValue("BumpScaling", [this] { return BumpScaling; });
    writeValue("Reflectivity", [this] { return Reflectivity; });
    writeValue("Refractivity", [this] { return Refractivity; });
    writeValue("Shininess", [this] { return Shininess; });
    writeValue("Shininess_Strength", [this] { return Shininess_Strength; });

    return w;
}

MaterialData::MaterialData()
{
    deviceModule = DeviceModule::getInstance();
    textureManager = TextureManager::getInstance();

    emptyTexture = textureManager->GetTexture("NULL_TEXTURE");

    texture_vector = std::make_shared<std::vector<std::shared_ptr<CustomTexture>>>();
    texture_vector->resize(TOTAL_NUM_TEXTURES, nullptr);

    // IDs por defecto
    IDTextures[TEXTURE_TYPE::DIFFUSE_TYPE] = -1;
    IDTextures[TEXTURE_TYPE::NORMAL_TYPE] = -1;
    IDTextures[TEXTURE_TYPE::SPECULAR_TYPE] = -1;
    IDTextures[TEXTURE_TYPE::EMISSIVE_TYPE] = -1;
    IDTextures[TEXTURE_TYPE::HEIGHT_TYPE] = -1;

    fillEmptyTextures();

    materialUBO = std::make_shared<UniformBufferObject>();

    for (uint32_t f = 0; f < MAX_FRAMES_IN_FLIGHT; ++f)
        isModified[f] = true;
}

void MaterialData::ImportAssimpMaterial(aiMaterial* material)
{
    if (!material) return;

    // Scalars
    material->Get(AI_MATKEY_OPACITY, Opacity);
    material->Get(AI_MATKEY_BUMPSCALING, BumpScaling);

    if (material->Get(AI_MATKEY_SHININESS, Shininess) == AI_SUCCESS)
    {
        if (Shininess == 0.0f) Shininess = 32.0f;
    }

    material->Get(AI_MATKEY_REFLECTIVITY, Reflectivity);
    material->Get(AI_MATKEY_SHININESS_STRENGTH, Shininess_Strength);
    material->Get(AI_MATKEY_REFRACTI, Refractivity);

    // Colors: Assimp -> aiColor4D -> glm::vec4
    aiColor4D c;

    if (material->Get(AI_MATKEY_COLOR_DIFFUSE, c) == AI_SUCCESS)     Diffuse = ToVec4(c);
    if (material->Get(AI_MATKEY_COLOR_AMBIENT, c) == AI_SUCCESS)     Ambient = ToVec4(c);
    if (material->Get(AI_MATKEY_COLOR_SPECULAR, c) == AI_SUCCESS)    Specular = ToVec4(c);
    if (material->Get(AI_MATKEY_COLOR_EMISSIVE, c) == AI_SUCCESS)    Emissive = ToVec4(c);
    if (material->Get(AI_MATKEY_COLOR_TRANSPARENT, c) == AI_SUCCESS) Transparent = ToVec4(c);
    if (material->Get(AI_MATKEY_COLOR_REFLECTIVE, c) == AI_SUCCESS)  Reflective = ToVec4(c);
}

void MaterialData::ImportAssimpTexture(const aiScene* scene, aiMaterial* material, std::string fileExtension_, std::string texturePath_)
{
    if (!scene || !material) return;

    texturePath = std::move(texturePath_);
    fileExtension = std::move(fileExtension_);

    // Diffuse/BaseColor/Roughness fallback chain
    {
        std::string textureName = GetTexture(scene, material, aiTextureType_DIFFUSE, TEXTURE_TYPE::DIFFUSE_TYPE);
        if (textureName.empty())
            textureName = GetTexture(scene, material, aiTextureType_BASE_COLOR, TEXTURE_TYPE::DIFFUSE_TYPE);
        if (textureName.empty())
            textureName = GetTexture(scene, material, aiTextureType_DIFFUSE_ROUGHNESS, TEXTURE_TYPE::DIFFUSE_TYPE);

        if (!textureName.empty())
            AddTexture(textureName, textureManager->GetTexture(textureName));
    }

    // Specular
    {
        std::string textureName = GetTexture(scene, material, aiTextureType_SPECULAR, TEXTURE_TYPE::SPECULAR_TYPE);
        if (!textureName.empty())
            AddTexture(textureName, textureManager->GetTexture(textureName));
    }

    // Normal (Normals o Height como fallback)
    {
        std::string textureName = GetTexture(scene, material, aiTextureType_NORMALS, TEXTURE_TYPE::NORMAL_TYPE);
        if (textureName.empty())
            textureName = GetTexture(scene, material, aiTextureType_HEIGHT, TEXTURE_TYPE::NORMAL_TYPE);

        if (!textureName.empty())
            AddTexture(textureName, textureManager->GetTexture(textureName));
    }

    // Emissive
    {
        std::string textureName = GetTexture(scene, material, aiTextureType_EMISSIVE, TEXTURE_TYPE::EMISSIVE_TYPE);
        if (!textureName.empty())
            AddTexture(textureName, textureManager->GetTexture(textureName));
    }

    // Height
    {
        std::string textureName = GetTexture(scene, material, aiTextureType_HEIGHT, TEXTURE_TYPE::HEIGHT_TYPE);
        if (!textureName.empty())
            AddTexture(textureName, textureManager->GetTexture(textureName));
    }
}

std::string MaterialData::GetTexture(const aiScene* scene, aiMaterial* mat, aiTextureType type, TEXTURE_TYPE textureType)
{
    (void)scene;

    aiString str;
    aiReturn texFound = mat->GetTexture(type, 0, &str);
    if (texFound != AI_SUCCESS)
        return "";

    std::string filePath = std::string(str.C_Str());
    std::string finalName;

    if (fileExtension == "fbx")
    {
        // FBX suele venir con backslashes
        std::size_t pos = filePath.find_last_of("\\/");
        finalName = (pos != std::string::npos) ? filePath.substr(pos + 1) : filePath;
    }
    else
    {
        finalName = filePath;
    }

    std::string fullPath = texturePath + finalName;

    if (currentTextures.find(str.C_Str()) == currentTextures.end())
    {
        finalName = textureManager->AddTexture(finalName, CustomTexture(fullPath, textureType));
        currentTextures.insert(str.C_Str());
    }

    return finalName;
}

std::shared_ptr<CustomTexture> MaterialData::findTextureByType(TEXTURE_TYPE newtype)
{
    if (newtype == TEXTURE_TYPE::NULL_TYPE)
        return textureManager->GetTexture("NULL_TEXTURE");

    for (size_t id = 0; id < TOTAL_NUM_TEXTURES; ++id)
    {
        auto& t = texture_vector->at(id);
        if (t && t->type == newtype)
            return t;
    }
    return nullptr;
}

void MaterialData::fillEmptyTextures()
{
    for (size_t i = 0; i < TOTAL_NUM_TEXTURES; ++i)
    {
        if (texture_vector->at(i) == nullptr)
            texture_vector->at(i) = emptyTexture;
    }
}

void MaterialData::AddTexture(const std::string& textureName, const std::shared_ptr<CustomTexture>& texture)
{
    if (!texture) return;

    int& slot = IDTextures[texture->type];
    if (slot >= 0 && slot < TOTAL_NUM_TEXTURES)
    {
        texture_vector->at(static_cast<size_t>(slot)) = texture;
    }
    else
    {
        if (numTextures < TOTAL_NUM_TEXTURES)
        {
            texture_vector->at(static_cast<size_t>(numTextures)) = texture;
            slot = numTextures;
            ++numTextures;
        }
    }

    textureManager->AddTexture(textureName, texture);
    Textures[texture->type] = texture;
}

void MaterialData::CleanLastResources()
{
    deviceModule = nullptr;
    textureManager = nullptr;
    currentTextures.clear();
    materialBuffer.clear();
    materialFields.clear();
    writers.clear();
}

void MaterialData::InitializeUBOMaterial(std::shared_ptr<ShaderModule> shader_ptr)
{
    if (!shader_ptr) return;

    auto& reflect = shader_ptr->reflectShader;
    if (!reflect.isUBOMaterial)
        return;

    materialBuffer.assign(reflect.materialBufferSize, 0u);

    writers = BuildMaterialWriters();

    materialFields.clear();
    materialFields.reserve(reflect.materialUBOMembers.size());

    for (const auto& mem : reflect.materialUBOMembers)
    {
        std::string key = NormalizeMaterialMemberName(mem.name);
        materialFields[key] = FieldInfo{ mem.offset, mem.size };

        auto it = writers.find(key);
        if (it != writers.end())
            it->second(materialBuffer.data(), mem.offset);
    }

    materialUniformSize = reflect.materialBufferSize;
    materialUBO->CreateUniformBuffer(materialUniformSize, MAX_FRAMES_IN_FLIGHT, *deviceModule);

    for (uint32_t f = 0; f < MAX_FRAMES_IN_FLIGHT; ++f)
        isModified[f] = true;

    UpdateUBOMaterial();
}

void MaterialData::UpdateUBOMaterial()
{
    if (!deviceModule || !materialUBO) return;
    if (materialUBO->uniformBuffers.empty()) return;
    if (materialUniformSize == 0) return;

    for (uint32_t currentFrame = 0; currentFrame < MAX_FRAMES_IN_FLIGHT; ++currentFrame)
    {
        if (!isModified[currentFrame]) continue;

        void* mapped = nullptr;
        vkMapMemory(deviceModule->device,
            materialUBO->uniformBuffersMemory[currentFrame],
            0, materialUniformSize, 0, &mapped);

        std::memcpy(mapped, materialBuffer.data(), static_cast<size_t>(materialUniformSize));

        vkUnmapMemory(deviceModule->device, materialUBO->uniformBuffersMemory[currentFrame]);
        isModified[currentFrame] = false;
    }
}

void MaterialData::CleanMaterialUBO()
{
    if (!deviceModule || !materialUBO) return;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        if (!materialUBO->uniformBuffers.empty() && materialUBO->uniformBuffers[i] != VK_NULL_HANDLE)
        {
            vkDestroyBuffer(deviceModule->device, materialUBO->uniformBuffers[i], nullptr);
            vkFreeMemory(deviceModule->device, materialUBO->uniformBuffersMemory[i], nullptr);
            materialUBO->uniformBuffers[i] = VK_NULL_HANDLE;
        }
    }
}

void MaterialData::WriteToMaterialBufferAt(const void* data, size_t offset, size_t size)
{
    if (offset + size > materialBuffer.size())
        throw std::runtime_error("MaterialData: write out of bounds");

    std::memcpy(materialBuffer.data() + offset, data, size);
}

void MaterialData::UpdateMaterialDataRaw(const std::string& fieldName, const void* src, size_t size)
{
    auto it = materialFields.find(fieldName);
    if (it == materialFields.end()) return;

    const FieldInfo& fi = it->second;
    const size_t writeSize = std::min(fi.size, size);

    WriteToMaterialBufferAt(src, fi.offset, writeSize);

    for (uint32_t f = 0; f < MAX_FRAMES_IN_FLIGHT; ++f)
        isModified[f] = true;
}

void MaterialData::SetMaterialField(const std::string& nameField_, float value)
{
    const std::string nameField = NormalizeMaterialMemberName(nameField_);

    if (nameField == "Opacity") { Opacity = value;            UpdateMaterialDataRaw("Opacity", &Opacity, sizeof(Opacity)); return; }
    if (nameField == "BumpScaling") { BumpScaling = value;        UpdateMaterialDataRaw("BumpScaling", &BumpScaling, sizeof(BumpScaling)); return; }
    if (nameField == "Shininess") { Shininess = value;          UpdateMaterialDataRaw("Shininess", &Shininess, sizeof(Shininess)); return; }
    if (nameField == "Reflectivity") { Reflectivity = value;       UpdateMaterialDataRaw("Reflectivity", &Reflectivity, sizeof(Reflectivity)); return; }
    if (nameField == "Shininess_Strength") { Shininess_Strength = value; UpdateMaterialDataRaw("Shininess_Strength", &Shininess_Strength, sizeof(Shininess_Strength)); return; }
    if (nameField == "Refractivity") { Refractivity = value;       UpdateMaterialDataRaw("Refractivity", &Refractivity, sizeof(Refractivity)); return; }
}

void MaterialData::SetMaterialField(const std::string& nameField_, glm::vec3 value)
{
    const std::string nameField = NormalizeMaterialMemberName(nameField_);
    glm::vec4 v(value, 1.0f);

    if (nameField == "Diffuse") { Diffuse = v;     UpdateMaterialDataRaw("Diffuse", &Diffuse, sizeof(Diffuse)); return; }
    if (nameField == "Ambient") { Ambient = v;     UpdateMaterialDataRaw("Ambient", &Ambient, sizeof(Ambient)); return; }
    if (nameField == "Specular") { Specular = v;    UpdateMaterialDataRaw("Specular", &Specular, sizeof(Specular)); return; }
    if (nameField == "Emissive") { Emissive = v;    UpdateMaterialDataRaw("Emissive", &Emissive, sizeof(Emissive)); return; }
    if (nameField == "Transparent") { Transparent = v; UpdateMaterialDataRaw("Transparent", &Transparent, sizeof(Transparent)); return; }
    if (nameField == "Reflective") { Reflective = v;  UpdateMaterialDataRaw("Reflective", &Reflective, sizeof(Reflective)); return; }
}

void MaterialData::SetMaterialField(const std::string& nameField_, int value)
{
    const std::string nameField = NormalizeMaterialMemberName(nameField_);

    if (nameField == "idxDiffuse") { IDTextures[TEXTURE_TYPE::DIFFUSE_TYPE] = value; UpdateMaterialDataRaw("idxDiffuse", &value, sizeof(value)); return; }
    if (nameField == "idxNormal") { IDTextures[TEXTURE_TYPE::NORMAL_TYPE] = value; UpdateMaterialDataRaw("idxNormal", &value, sizeof(value)); return; }
    if (nameField == "idxSpecular") { IDTextures[TEXTURE_TYPE::SPECULAR_TYPE] = value; UpdateMaterialDataRaw("idxSpecular", &value, sizeof(value)); return; }
    if (nameField == "idxEmissive") { IDTextures[TEXTURE_TYPE::EMISSIVE_TYPE] = value; UpdateMaterialDataRaw("idxEmissive", &value, sizeof(value)); return; }
    if (nameField == "idxHeight") { IDTextures[TEXTURE_TYPE::HEIGHT_TYPE] = value; UpdateMaterialDataRaw("idxHeight", &value, sizeof(value)); return; }
}
