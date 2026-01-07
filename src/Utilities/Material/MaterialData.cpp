#include "MaterialData.h"

static uint32_t SlotBitFromType(TEXTURE_TYPE t)
{
    switch (t)
    {
    case TEXTURE_TYPE::DIFFUSE_TYPE:   return 1u << 0; // QE_SLOT_BASECOLOR
    case TEXTURE_TYPE::NORMAL_TYPE:    return 1u << 1;
    case TEXTURE_TYPE::METALNESS_TYPE: return 1u << 2;
    case TEXTURE_TYPE::ROUGHNESS_TYPE: return 1u << 3;
    case TEXTURE_TYPE::AO_TYPE:        return 1u << 4;
    case TEXTURE_TYPE::EMISSIVE_TYPE:  return 1u << 5;
    case TEXTURE_TYPE::HEIGHT_TYPE:    return 1u << 6;
    case TEXTURE_TYPE::SPECULAR_TYPE:  return 1u << 7;
    default: return 0;
    }
}

static inline QEColorSpace ColorSpaceFromSemantic(TEXTURE_TYPE t)
{
    switch (t)
    {
    case TEXTURE_TYPE::DIFFUSE_TYPE:
    case TEXTURE_TYPE::EMISSIVE_TYPE:
        return QEColorSpace::SRGB;

    default:
        return QEColorSpace::Linear; // normal, metal, rough, ao, height, spec...
    }
}

static std::string& PathRefForSemantic(MaterialData& md, TEXTURE_TYPE s)
{
    switch (s)
    {
    case TEXTURE_TYPE::DIFFUSE_TYPE:   return md.diffuseTexturePath;
    case TEXTURE_TYPE::NORMAL_TYPE:    return md.normalTexturePath;
    case TEXTURE_TYPE::METALNESS_TYPE: return md.metallicTexturePath;
    case TEXTURE_TYPE::ROUGHNESS_TYPE: return md.roughnessTexturePath;
    case TEXTURE_TYPE::AO_TYPE:        return md.aoTexturePath;
    case TEXTURE_TYPE::EMISSIVE_TYPE:  return md.emissiveTexturePath;
    case TEXTURE_TYPE::HEIGHT_TYPE:    return md.heightTexturePath;
    case TEXTURE_TYPE::SPECULAR_TYPE:  return md.specularTexturePath;
    default: return md.diffuseTexturePath; // no debería usarse
    }
}

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

    // ints (UBO indices reales)
    writeValue("idxDiffuse", [this] { return idxDiffuse; });
    writeValue("idxNormal", [this] { return idxNormal; });
    writeValue("idxSpecular", [this] { return idxSpecular; });
    writeValue("idxEmissive", [this] { return idxEmissive; });
    writeValue("idxHeight", [this] { return idxHeight; });
    writeValue("idxMetallic", [this] { return idxMetallic; });
    writeValue("idxRoughness", [this] { return idxRoughness; });
    writeValue("idxAO", [this] { return idxAO; });

    //uints
    writeValue("texMask", [this] { return TexMask; });
    writeValue("metallicChan", [this] { return MetallicChan; });
    writeValue("roughnessChan", [this] { return RoughnessChan; });
    writeValue("aoChan", [this] { return AOChan; });

    // floats
    writeValue("Opacity", [this] { return Opacity; });
    writeValue("BumpScaling", [this] { return BumpScaling; });
    writeValue("Reflectivity", [this] { return Reflectivity; });
    writeValue("Refractivity", [this] { return Refractivity; });
    writeValue("Shininess", [this] { return Shininess; });
    writeValue("Shininess_Strength", [this] { return Shininess_Strength; });
    writeValue("Metallic", [this] { return Metallic; });
    writeValue("Roughness", [this] { return Roughness; });
    writeValue("AO", [this] { return AO; });
    writeValue("Clearcoat", [this] { return Clearcoat; });
    writeValue("ClearcoatRoughness", [this] { return ClearcoatRoughness; });

    return w;
}

MaterialData::MaterialData()
{
    deviceModule = DeviceModule::getInstance();
    textureManager = TextureManager::getInstance();

    emptyTexture = textureManager->GetTexture("NULL_TEXTURE");

    texture_vector = std::make_shared<std::vector<std::shared_ptr<CustomTexture>>>();
    texture_vector->resize(TOTAL_NUM_TEXTURES, nullptr);

    TexMask = 0;
    MetallicChan = 0;
    RoughnessChan = 0;
    AOChan = 0;

    // Índices (slots)
    IDTextures[TEXTURE_TYPE::DIFFUSE_TYPE] = (int)MAT_TEX_SLOT::BaseColor;
    IDTextures[TEXTURE_TYPE::NORMAL_TYPE] = (int)MAT_TEX_SLOT::Normal;
    IDTextures[TEXTURE_TYPE::EMISSIVE_TYPE] = (int)MAT_TEX_SLOT::Emissive;
    IDTextures[TEXTURE_TYPE::HEIGHT_TYPE] = (int)MAT_TEX_SLOT::Height;

    IDTextures[TEXTURE_TYPE::SPECULAR_TYPE] = (int)MAT_TEX_SLOT::Reserved7;

    // PBR
    IDTextures[TEXTURE_TYPE::METALNESS_TYPE] = (int)MAT_TEX_SLOT::Metallic;
    IDTextures[TEXTURE_TYPE::ROUGHNESS_TYPE] = (int)MAT_TEX_SLOT::Roughness;
    IDTextures[TEXTURE_TYPE::AO_TYPE] = (int)MAT_TEX_SLOT::AO;

    fillEmptyTextures();

    materialUBO = std::make_shared<UniformBufferObject>();

    for (uint32_t f = 0; f < MAX_FRAMES_IN_FLIGHT; ++f)
        isModified[f] = true;
}

void MaterialData::ImportAssimpMaterial(aiMaterial* material)
{
    if (!material) return;

    if (Clearcoat < 0.0f) Clearcoat = 0.0f;
    if (ClearcoatRoughness <= 0.0f) ClearcoatRoughness = 0.1f;

    // --- Scalars (legacy / Phong) ---
    material->Get(AI_MATKEY_OPACITY, Opacity);
    material->Get(AI_MATKEY_BUMPSCALING, BumpScaling);

    if (material->Get(AI_MATKEY_SHININESS, Shininess) == AI_SUCCESS)
    {
        if (Shininess == 0.0f) Shininess = 32.0f;
    }

    material->Get(AI_MATKEY_REFLECTIVITY, Reflectivity);
    material->Get(AI_MATKEY_SHININESS_STRENGTH, Shininess_Strength);
    material->Get(AI_MATKEY_REFRACTI, Refractivity);

    // --- PBR: Clearcoat (KHR_materials_clearcoat, etc.) ---
    {
        float cc = Clearcoat;
        if (material->Get(AI_MATKEY_CLEARCOAT_FACTOR, cc) == AI_SUCCESS)
        {
            if (cc < 0.0f) cc = 0.0f;
            if (cc > 1.0f) cc = 1.0f;
            Clearcoat = cc;
        }

        float ccr = ClearcoatRoughness;
        if (material->Get(AI_MATKEY_CLEARCOAT_ROUGHNESS_FACTOR, ccr) == AI_SUCCESS)
        {
            if (ccr < 0.03f) ccr = 0.03f;
            if (ccr > 1.0f)  ccr = 1.0f;
            ClearcoatRoughness = ccr;
        }
    }

    // --- Colors: Assimp -> aiColor4D -> glm::vec4 ---
    aiColor4D c;
    if (material->Get(AI_MATKEY_COLOR_DIFFUSE, c) == AI_SUCCESS)     Diffuse = ToVec4(c);
    if (material->Get(AI_MATKEY_COLOR_AMBIENT, c) == AI_SUCCESS)     Ambient = ToVec4(c);
    if (material->Get(AI_MATKEY_COLOR_SPECULAR, c) == AI_SUCCESS)    Specular = ToVec4(c);
    if (material->Get(AI_MATKEY_COLOR_EMISSIVE, c) == AI_SUCCESS)    Emissive = ToVec4(c);
    if (material->Get(AI_MATKEY_COLOR_TRANSPARENT, c) == AI_SUCCESS) Transparent = ToVec4(c);
    if (material->Get(AI_MATKEY_COLOR_REFLECTIVE, c) == AI_SUCCESS)  Reflective = ToVec4(c);
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

std::shared_ptr<CustomTexture> MaterialData::findTextureByType(TEXTURE_TYPE t)
{
    if (t == TEXTURE_TYPE::NULL_TYPE)
        return emptyTexture;

    int slot = SlotFromType(t);
    if (slot < 0 || slot >= TOTAL_NUM_TEXTURES)
        return nullptr;

    auto& tex = texture_vector->at((size_t)slot);
    return tex ? tex : emptyTexture;
}

void MaterialData::fillEmptyTextures()
{
    for (size_t i = 0; i < TOTAL_NUM_TEXTURES; ++i)
    {
        if (texture_vector->at(i) == nullptr)
            texture_vector->at(i) = emptyTexture;
    }
}

void MaterialData::AddTexture(TEXTURE_TYPE semantic, const std::string& texturePath)
{
    PathRefForSemantic(*this, semantic) =
        (texturePath.empty() ? "NULL_TEXTURE" : texturePath);

    auto it = IDTextures.find(semantic);
    if (it == IDTextures.end()) return;

    int slot = it->second;
    if (slot < 0 || slot >= TOTAL_NUM_TEXTURES) return;

    std::shared_ptr<CustomTexture> tex;

    if (texturePath.empty() || texturePath == "NULL_TEXTURE" || (!texturePath.empty() && texturePath[0] == '*'))
    {
        tex = textureManager->GetTexture("NULL_TEXTURE");
    }
    else
    {
        QEColorSpace cs = ColorSpaceFromSemantic(semantic);
        tex = textureManager->GetOrLoadTextureByPath(texturePath, cs);
    }

    if (!tex) return;

    texture_vector->at((size_t)slot) = tex;
    Textures[semantic] = tex;

    bool isReal = !texturePath.empty() && texturePath != "NULL_TEXTURE" && texturePath[0] != '*';
    if (isReal)
    {
        uint32_t bit = SlotBitFromType(semantic);
        if (bit)
        {
            TexMask |= bit;
            UpdateMaterialDataRaw("texMask", &TexMask, sizeof(TexMask));
        }
    }

    auto m = findTextureByType(TEXTURE_TYPE::METALNESS_TYPE);
    auto r = findTextureByType(TEXTURE_TYPE::ROUGHNESS_TYPE);

    if (m && r && m == r && m != emptyTexture)
    {
        idxMetallic = (int)MAT_TEX_SLOT::Metallic;
        idxRoughness = idxMetallic;

        MetallicChan = 2;
        RoughnessChan = 1;

        UpdateMaterialDataRaw("idxMetallic", &idxMetallic, sizeof(idxMetallic));
        UpdateMaterialDataRaw("idxRoughness", &idxRoughness, sizeof(idxRoughness));
        UpdateMaterialDataRaw("metallicChan", &MetallicChan, sizeof(MetallicChan));
        UpdateMaterialDataRaw("roughnessChan", &RoughnessChan, sizeof(RoughnessChan));

        TexMask |= (1u << (uint32_t)MAT_TEX_SLOT::Metallic);
        TexMask |= (1u << (uint32_t)MAT_TEX_SLOT::Roughness);
        UpdateMaterialDataRaw("texMask", &TexMask, sizeof(TexMask));
    }

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
    if (nameField == "Metallic") { Metallic = value;  UpdateMaterialDataRaw("Metallic", &Metallic, sizeof(Metallic)); return; }
    if (nameField == "Roughness") { Roughness = value; UpdateMaterialDataRaw("Roughness", &Roughness, sizeof(Roughness)); return; }
    if (nameField == "AO") { AO = value;        UpdateMaterialDataRaw("AO", &AO, sizeof(AO)); return; }
    if (nameField == "Clearcoat") { Clearcoat = value;        UpdateMaterialDataRaw("Clearcoat", &Clearcoat, sizeof(Clearcoat)); return; }
    if (nameField == "ClearcoatRoughness") { ClearcoatRoughness = value;        UpdateMaterialDataRaw("ClearcoatRoughness", &ClearcoatRoughness, sizeof(ClearcoatRoughness)); return; }
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

    if (nameField == "idxDiffuse") { idxDiffuse = value;   UpdateMaterialDataRaw("idxDiffuse", &idxDiffuse, sizeof(idxDiffuse)); return; }
    if (nameField == "idxNormal") { idxNormal = value;    UpdateMaterialDataRaw("idxNormal", &idxNormal, sizeof(idxNormal)); return; }
    if (nameField == "idxSpecular") { idxSpecular = value;  UpdateMaterialDataRaw("idxSpecular", &idxSpecular, sizeof(idxSpecular)); return; }
    if (nameField == "idxEmissive") { idxEmissive = value;  UpdateMaterialDataRaw("idxEmissive", &idxEmissive, sizeof(idxEmissive)); return; }
    if (nameField == "idxHeight") { idxHeight = value;    UpdateMaterialDataRaw("idxHeight", &idxHeight, sizeof(idxHeight)); return; }
    if (nameField == "idxMetallic") { idxMetallic = value;  UpdateMaterialDataRaw("idxMetallic", &idxMetallic, sizeof(idxMetallic)); return; }
    if (nameField == "idxRoughness") { idxRoughness = value; UpdateMaterialDataRaw("idxRoughness", &idxRoughness, sizeof(idxRoughness)); return; }
    if (nameField == "idxAO") { idxAO = value;        UpdateMaterialDataRaw("idxAO", &idxAO, sizeof(idxAO)); return; }
}

void MaterialData::SetTextureSlot(uint32_t slot, const std::shared_ptr<CustomTexture>& tex, bool isReal)
{
    texture_vector->at(slot) = tex;
    if (isReal)
    {
        TexMask |= (1u << slot);
        UpdateMaterialDataRaw("texMask", &TexMask, sizeof(TexMask));
    }
}

void MaterialData::ApplyDtoPacking(const MaterialDto& dto)
{
    MetallicChan = dto.metallicChan;
    RoughnessChan = dto.roughnessChan;
    AOChan = dto.aoChan;

    UpdateMaterialDataRaw("metallicChan", &MetallicChan, sizeof(MetallicChan));
    UpdateMaterialDataRaw("roughnessChan", &RoughnessChan, sizeof(RoughnessChan));
    UpdateMaterialDataRaw("aoChan", &AOChan, sizeof(AOChan));

    TexMask = dto.texMask;
    UpdateMaterialDataRaw("texMask", &TexMask, sizeof(TexMask));

    if (dto.metallicTexturePath != "NULL_TEXTURE" &&
        dto.roughnessTexturePath != "NULL_TEXTURE" &&
        !dto.metallicTexturePath.empty() &&
        dto.metallicTexturePath == dto.roughnessTexturePath)
    {
        idxMetallic = (int)MAT_TEX_SLOT::Metallic;
        idxRoughness = idxMetallic;

        // glTF default
        MetallicChan = (dto.metallicChan != 0u) ? dto.metallicChan : 2u;
        RoughnessChan = (dto.roughnessChan != 0u) ? dto.roughnessChan : 1u;

        UpdateMaterialDataRaw("idxMetallic", &idxMetallic, sizeof(idxMetallic));
        UpdateMaterialDataRaw("idxRoughness", &idxRoughness, sizeof(idxRoughness));
        UpdateMaterialDataRaw("metallicChan", &MetallicChan, sizeof(MetallicChan));
        UpdateMaterialDataRaw("roughnessChan", &RoughnessChan, sizeof(RoughnessChan));

        TexMask |= (1u << (uint32_t)MAT_TEX_SLOT::Metallic);
        TexMask |= (1u << (uint32_t)MAT_TEX_SLOT::Roughness);
        UpdateMaterialDataRaw("texMask", &TexMask, sizeof(TexMask));
    }
}
