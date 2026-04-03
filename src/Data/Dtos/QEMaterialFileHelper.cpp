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
