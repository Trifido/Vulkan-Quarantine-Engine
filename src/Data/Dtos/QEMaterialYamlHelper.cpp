#include "QEMaterialYamlHelper.h"

#include <fstream>
#include <iostream>
#include <system_error>

namespace fs = std::filesystem;

YAML::Node QEMaterialYamlHelper::SerializeVec4(const glm::vec4& value)
{
    YAML::Node node;
    node.push_back(value.x);
    node.push_back(value.y);
    node.push_back(value.z);
    node.push_back(value.w);
    return node;
}

glm::vec4 QEMaterialYamlHelper::DeserializeVec4(const YAML::Node& node, const glm::vec4& defaultValue)
{
    if (!node || !node.IsSequence() || node.size() != 4)
        return defaultValue;

    return glm::vec4(
        node[0].as<float>(),
        node[1].as<float>(),
        node[2].as<float>(),
        node[3].as<float>()
    );
}

std::string QEMaterialYamlHelper::NormalizeYamlTexturePath(const std::string& path)
{
    // Respetamos tu sentinel interno
    if (IsNullTex(path))
        return "NULL_TEXTURE";

    return NormalizeSlashes(path);
}

YAML::Node QEMaterialYamlHelper::SerializeMaterialDto(const MaterialDto& dto)
{
    YAML::Node root;
    YAML::Node material;

    material["Name"] = dto.Name;
    material["FilePath"] = NormalizeSlashes(dto.FilePath);
    material["ShaderPath"] = dto.ShaderPath;
    material["Layer"] = dto.layer;

    YAML::Node scalars;
    scalars["Opacity"] = dto.Opacity;
    scalars["BumpScaling"] = dto.BumpScaling;
    scalars["Shininess"] = dto.Shininess;
    scalars["Reflectivity"] = dto.Reflectivity;
    scalars["Shininess_Strength"] = dto.Shininess_Strength;
    scalars["Refractivity"] = dto.Refractivity;
    scalars["Metallic"] = dto.Metallic;
    scalars["Roughness"] = dto.Roughness;
    scalars["AO"] = dto.AO;
    scalars["Clearcoat"] = dto.Clearcoat;
    scalars["ClearcoatRoughness"] = dto.ClearcoatRoughness;
    scalars["AlphaCutoff"] = dto.AlphaCutoff;
    scalars["AlphaMode"] = dto.AlphaMode;
    material["Scalars"] = scalars;

    YAML::Node colors;
    colors["Diffuse"] = SerializeVec4(dto.Diffuse);
    colors["Ambient"] = SerializeVec4(dto.Ambient);
    colors["Specular"] = SerializeVec4(dto.Specular);
    colors["Emissive"] = SerializeVec4(dto.Emissive);
    colors["Transparent"] = SerializeVec4(dto.Transparent);
    colors["Reflective"] = SerializeVec4(dto.Reflective);
    material["Colors"] = colors;

    YAML::Node packing;
    packing["TexMask"] = dto.texMask;
    packing["MetallicChan"] = dto.metallicChan;
    packing["RoughnessChan"] = dto.roughnessChan;
    packing["AOChan"] = dto.aoChan;
    material["TexturePacking"] = packing;

    YAML::Node textures;
    textures["Diffuse"] = NormalizeYamlTexturePath(dto.diffuseTexturePath);
    textures["Normal"] = NormalizeYamlTexturePath(dto.normalTexturePath);
    textures["Specular"] = NormalizeYamlTexturePath(dto.specularTexturePath);
    textures["Emissive"] = NormalizeYamlTexturePath(dto.emissiveTexturePath);
    textures["Height"] = NormalizeYamlTexturePath(dto.heightTexturePath);
    textures["Metallic"] = NormalizeYamlTexturePath(dto.metallicTexturePath);
    textures["Roughness"] = NormalizeYamlTexturePath(dto.roughnessTexturePath);
    textures["AO"] = NormalizeYamlTexturePath(dto.aoTexturePath);
    material["Textures"] = textures;

    root["Material"] = material;
    return root;
}

bool QEMaterialYamlHelper::DeserializeMaterialDto(const YAML::Node& root, MaterialDto& dto)
{
    const YAML::Node material = root["Material"];
    if (!material)
        return false;

    dto = MaterialDto{};

    dto.Name = material["Name"] ? material["Name"].as<std::string>() : "";
    dto.FilePath = material["FilePath"] ? NormalizeSlashes(material["FilePath"].as<std::string>()) : "";
    dto.ShaderPath = material["ShaderPath"] ? material["ShaderPath"].as<std::string>() : "";
    dto.layer = material["Layer"] ? material["Layer"].as<int>() : 1;

    if (const YAML::Node scalars = material["Scalars"])
    {
        dto.Opacity = scalars["Opacity"] ? scalars["Opacity"].as<float>() : 1.0f;
        dto.BumpScaling = scalars["BumpScaling"] ? scalars["BumpScaling"].as<float>() : 1.0f;
        dto.Shininess = scalars["Shininess"] ? scalars["Shininess"].as<float>() : 32.0f;
        dto.Reflectivity = scalars["Reflectivity"] ? scalars["Reflectivity"].as<float>() : 0.0f;
        dto.Shininess_Strength = scalars["Shininess_Strength"] ? scalars["Shininess_Strength"].as<float>() : 0.0f;
        dto.Refractivity = scalars["Refractivity"] ? scalars["Refractivity"].as<float>() : 0.0f;
        dto.Metallic = scalars["Metallic"] ? scalars["Metallic"].as<float>() : 0.0f;
        dto.Roughness = scalars["Roughness"] ? scalars["Roughness"].as<float>() : 1.0f;
        dto.AO = scalars["AO"] ? scalars["AO"].as<float>() : 1.0f;
        dto.Clearcoat = scalars["Clearcoat"] ? scalars["Clearcoat"].as<float>() : 0.0f;
        dto.ClearcoatRoughness = scalars["ClearcoatRoughness"] ? scalars["ClearcoatRoughness"].as<float>() : 0.1f;
        dto.AlphaCutoff = scalars["AlphaCutoff"] ? scalars["AlphaCutoff"].as<float>() : 0.5f;
        dto.AlphaMode = scalars["AlphaMode"] ? scalars["AlphaMode"].as<uint32_t>() : 0u;
    }

    if (const YAML::Node colors = material["Colors"])
    {
        dto.Diffuse = DeserializeVec4(colors["Diffuse"], glm::vec4(1.0f));
        dto.Ambient = DeserializeVec4(colors["Ambient"], glm::vec4(0.1f));
        dto.Specular = DeserializeVec4(colors["Specular"], glm::vec4(1.0f));
        dto.Emissive = DeserializeVec4(colors["Emissive"], glm::vec4(0.0f));
        dto.Transparent = DeserializeVec4(colors["Transparent"], glm::vec4(1.0f));
        dto.Reflective = DeserializeVec4(colors["Reflective"], glm::vec4(0.0f));
    }

    if (const YAML::Node packing = material["TexturePacking"])
    {
        dto.texMask = packing["TexMask"] ? packing["TexMask"].as<uint32_t>() : 0u;
        dto.metallicChan = packing["MetallicChan"] ? packing["MetallicChan"].as<uint32_t>() : 0u;
        dto.roughnessChan = packing["RoughnessChan"] ? packing["RoughnessChan"].as<uint32_t>() : 0u;
        dto.aoChan = packing["AOChan"] ? packing["AOChan"].as<uint32_t>() : 0u;
    }

    if (const YAML::Node textures = material["Textures"])
    {
        dto.diffuseTexturePath = textures["Diffuse"] ? NormalizeYamlTexturePath(textures["Diffuse"].as<std::string>()) : "NULL_TEXTURE";
        dto.normalTexturePath = textures["Normal"] ? NormalizeYamlTexturePath(textures["Normal"].as<std::string>()) : "NULL_TEXTURE";
        dto.specularTexturePath = textures["Specular"] ? NormalizeYamlTexturePath(textures["Specular"].as<std::string>()) : "NULL_TEXTURE";
        dto.emissiveTexturePath = textures["Emissive"] ? NormalizeYamlTexturePath(textures["Emissive"].as<std::string>()) : "NULL_TEXTURE";
        dto.heightTexturePath = textures["Height"] ? NormalizeYamlTexturePath(textures["Height"].as<std::string>()) : "NULL_TEXTURE";
        dto.metallicTexturePath = textures["Metallic"] ? NormalizeYamlTexturePath(textures["Metallic"].as<std::string>()) : "NULL_TEXTURE";
        dto.roughnessTexturePath = textures["Roughness"] ? NormalizeYamlTexturePath(textures["Roughness"].as<std::string>()) : "NULL_TEXTURE";
        dto.aoTexturePath = textures["AO"] ? NormalizeYamlTexturePath(textures["AO"].as<std::string>()) : "NULL_TEXTURE";
    }

    return true;
}

bool QEMaterialYamlHelper::WriteMaterialFile(const fs::path& filePath, const MaterialDto& dto)
{
    std::error_code ec;
    fs::create_directories(filePath.parent_path(), ec);
    if (ec)
    {
        std::cerr << "Error creando directorio para material: "
            << filePath.parent_path() << " -> " << ec.message() << std::endl;
        return false;
    }

    YAML::Emitter out;
    out << SerializeMaterialDto(dto);

    std::ofstream file(filePath, std::ios::out | std::ios::trunc);
    if (!file.is_open())
    {
        std::cerr << "Error al abrir " << filePath << " para escritura." << std::endl;
        return false;
    }

    file << out.c_str();
    file.close();
    return true;
}

bool QEMaterialYamlHelper::ReadMaterialFile(const fs::path& filePath, MaterialDto& dto)
{
    try
    {
        const YAML::Node root = YAML::LoadFile(filePath.string());
        return DeserializeMaterialDto(root, dto);
    }
    catch (const YAML::BadFile& e)
    {
        std::cerr << "No se pudo abrir el material: " << filePath << " (" << e.what() << ")\n";
        return false;
    }
    catch (const YAML::ParserException& e)
    {
        std::cerr << "YAML inválido en material " << filePath << " (" << e.what() << ")\n";
        return false;
    }
}
