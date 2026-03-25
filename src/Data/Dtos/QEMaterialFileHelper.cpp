#include "QEMaterialFileHelper.h"

#include <fstream>
#include <iostream>
#include <system_error>

#include <glm/glm.hpp>

// Ajusta estos includes según dónde tengas enums / constantes
#include "Material.h"          // si aquí está RenderLayer
#include "MaterialData.h"      // si aquí están máscaras/canales
// o donde tengas RenderLayer, etc.

namespace fs = std::filesystem;

std::string QEMaterialFileHelper::ToProjectRelativePath(
    const fs::path& absolutePath,
    const fs::path& projectRoot)
{
    std::error_code ec;
    fs::path rel = fs::relative(absolutePath, projectRoot, ec);

    if (ec)
        return absolutePath.lexically_normal().generic_string();

    return rel.lexically_normal().generic_string();
}

MaterialDto QEMaterialFileHelper::BuildBaseMaterialDto(
    const std::string& materialName,
    const std::string& shaderName,
    const fs::path& projectRoot)
{
    MaterialDto dto{};

    const fs::path absoluteMaterialPath =
        projectRoot / "QEAssets" / "QEMaterials" / (materialName + ".qemat");

    dto.Name = materialName;
    dto.FilePath = ToProjectRelativePath(absoluteMaterialPath, projectRoot);
    dto.ShaderPath = shaderName;

    dto.layer = (int)RenderLayer::SOLID;

    // Scalars legacy
    dto.Opacity = 1.0f;
    dto.BumpScaling = 1.0f;
    dto.Shininess = 32.0f;
    dto.Reflectivity = 0.0f;
    dto.Shininess_Strength = 1.0f;
    dto.Refractivity = 1.0f;

    // PBR
    dto.Metallic = 0.0f;
    dto.Roughness = 1.0f;
    dto.AO = 1.0f;
    dto.Clearcoat = 0.0f;
    dto.ClearcoatRoughness = 0.0f;
    dto.AlphaCutoff = 0.5f;

    // Colors
    dto.Diffuse = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    dto.Ambient = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    dto.Specular = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    dto.Emissive = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    dto.Transparent = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
    dto.Reflective = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);

    // Mask + channels
    dto.texMask = 0u;
    dto.metallicChan = 2u;
    dto.roughnessChan = 1u;
    dto.aoChan = 0u;

    // Textures vacías
    dto.diffuseTexturePath.clear();
    dto.normalTexturePath.clear();
    dto.metallicTexturePath.clear();
    dto.roughnessTexturePath.clear();
    dto.aoTexturePath.clear();
    dto.emissiveTexturePath.clear();
    dto.heightTexturePath.clear();
    dto.specularTexturePath.clear();

    return dto;
}

MaterialDto QEMaterialFileHelper::BuildDefaultPrimitiveMaterialDto(const fs::path& projectRoot)
{
    MaterialDto dto = BuildBaseMaterialDto("defaultPrimitiveMat", "default_primitive", projectRoot);
    dto.layer = (int)RenderLayer::SOLID;
    return dto;
}

MaterialDto QEMaterialFileHelper::BuildDefaultParticlesMaterialDto(const fs::path& projectRoot)
{
    MaterialDto dto = BuildBaseMaterialDto("defaultParticlesMat", "default_particles", projectRoot);
    dto.layer = (int)RenderLayer::PARTICLES;

    // Si quieres, puedes dejar algunos defaults distintos
    dto.Diffuse = glm::vec4(1.0f);
    dto.Emissive = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

    return dto;
}

MaterialDto QEMaterialFileHelper::BuildEditorDebugAABBMaterialDto(const fs::path& projectRoot)
{
    MaterialDto dto = BuildBaseMaterialDto("editorDebugAABB", "shader_aabb_debug", projectRoot);
    dto.layer = (int)RenderLayer::SOLID;

    dto.Diffuse = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f); // opcional
    return dto;
}

MaterialDto QEMaterialFileHelper::BuildEditorDebugColliderMaterialDto(const fs::path& projectRoot)
{
    MaterialDto dto = BuildBaseMaterialDto("editorDebugCollider", "shader_debug_lines", projectRoot);
    dto.layer = (int)RenderLayer::SOLID;

    dto.Diffuse = glm::vec4(1.0f, 0.5f, 0.0f, 1.0f); // opcional
    return dto;
}

MaterialDto QEMaterialFileHelper::BuildEditorGridMaterialDto(const fs::path& projectRoot)
{
    MaterialDto dto = BuildBaseMaterialDto("editorGrid", "shader_grid", projectRoot);
    dto.layer = (int)RenderLayer::SOLID;

    dto.Diffuse = glm::vec4(1.0f);
    return dto;
}

bool QEMaterialFileHelper::WriteMaterialFile(const fs::path& filePath, const MaterialDto& dto)
{
    std::error_code ec;
    fs::create_directories(filePath.parent_path(), ec);

    if (ec)
    {
        std::cerr << "Error creando directorio para material: "
            << filePath.parent_path() << " -> " << ec.message() << std::endl;
        return false;
    }

    std::ofstream file(filePath, std::ios::binary | std::ios::trunc);
    if (!file.is_open())
    {
        std::cerr << "Error al abrir " << filePath << " para escritura." << std::endl;
        return false;
    }

    auto writeString = [&](const std::string& s)
        {
            int len = static_cast<int>(s.size());
            file.write(reinterpret_cast<const char*>(&len), sizeof(int));
            if (len > 0)
                file.write(s.data(), len);
        };

    // Header
    writeString(dto.Name);
    writeString(dto.FilePath);
    writeString(dto.ShaderPath);

    // Layer
    file.write(reinterpret_cast<const char*>(&dto.layer), sizeof(int));

    // Scalars legacy
    file.write(reinterpret_cast<const char*>(&dto.Opacity), sizeof(float));
    file.write(reinterpret_cast<const char*>(&dto.BumpScaling), sizeof(float));
    file.write(reinterpret_cast<const char*>(&dto.Shininess), sizeof(float));
    file.write(reinterpret_cast<const char*>(&dto.Reflectivity), sizeof(float));
    file.write(reinterpret_cast<const char*>(&dto.Shininess_Strength), sizeof(float));
    file.write(reinterpret_cast<const char*>(&dto.Refractivity), sizeof(float));

    // PBR
    file.write(reinterpret_cast<const char*>(&dto.Metallic), sizeof(float));
    file.write(reinterpret_cast<const char*>(&dto.Roughness), sizeof(float));
    file.write(reinterpret_cast<const char*>(&dto.AO), sizeof(float));
    file.write(reinterpret_cast<const char*>(&dto.Clearcoat), sizeof(float));
    file.write(reinterpret_cast<const char*>(&dto.ClearcoatRoughness), sizeof(float));

    // Colors
    file.write(reinterpret_cast<const char*>(&dto.Diffuse), sizeof(glm::vec4));
    file.write(reinterpret_cast<const char*>(&dto.Ambient), sizeof(glm::vec4));
    file.write(reinterpret_cast<const char*>(&dto.Specular), sizeof(glm::vec4));
    file.write(reinterpret_cast<const char*>(&dto.Emissive), sizeof(glm::vec4));
    file.write(reinterpret_cast<const char*>(&dto.Transparent), sizeof(glm::vec4));
    file.write(reinterpret_cast<const char*>(&dto.Reflective), sizeof(glm::vec4));

    // Mask + channels
    file.write(reinterpret_cast<const char*>(&dto.texMask), sizeof(uint32_t));
    file.write(reinterpret_cast<const char*>(&dto.metallicChan), sizeof(uint32_t));
    file.write(reinterpret_cast<const char*>(&dto.roughnessChan), sizeof(uint32_t));
    file.write(reinterpret_cast<const char*>(&dto.aoChan), sizeof(uint32_t));

    // 8 texture slots
    writeString(dto.diffuseTexturePath);
    writeString(dto.normalTexturePath);
    writeString(dto.metallicTexturePath);
    writeString(dto.roughnessTexturePath);
    writeString(dto.aoTexturePath);
    writeString(dto.emissiveTexturePath);
    writeString(dto.heightTexturePath);
    writeString(dto.specularTexturePath);

    file.close();
    return true;
}
