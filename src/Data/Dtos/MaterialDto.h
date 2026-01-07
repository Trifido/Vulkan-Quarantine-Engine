#pragma once
#ifndef MATERIAL_DTO_H
#define MATERIAL_DTO_H

#include <glm/glm.hpp>
#include <string>
#include <filesystem>

namespace fs = std::filesystem;

static inline bool IsNullTex(const std::string& p)
{
    return p.empty() || p == "NULL_TEXTURE";
}

static inline bool IsAbsolutePath(const std::string& p)
{
    // fs::path::is_absolute funciona para Windows y Unix
    return fs::path(p).is_absolute();
}

static inline std::string NormalizeSlashes(std::string p)
{
    for (auto& c : p)
        if (c == '\\') c = '/';
    return p;
}

struct MaterialDto
{
    std::string Name = "";
    std::string FilePath = "";
    std::string ShaderPath = "";
    int layer = 1;

    float Opacity = 1.0f;
    float BumpScaling = 1.0f;
    float Shininess = 32.0f;
    float Reflectivity = 0.0f;
    float Shininess_Strength = 0.0f;
    float Refractivity = 0.0f;
    float Metallic = 0.0f;
    float Roughness = 1.0f;
    float AO = 1.0f;

    glm::vec4 Diffuse = glm::vec4(1.0f);
    glm::vec4 Ambient = glm::vec4(0.1f);
    glm::vec4 Specular = glm::vec4(1.0f);
    glm::vec4 Emissive = glm::vec4(0.0f);
    glm::vec4 Transparent = glm::vec4(1.0f);
    glm::vec4 Reflective = glm::vec4(0.0f);

    std::string diffuseTexturePath = "NULL_TEXTURE";
    std::string normalTexturePath = "NULL_TEXTURE";
    std::string specularTexturePath = "NULL_TEXTURE";
    std::string emissiveTexturePath = "NULL_TEXTURE";
    std::string heightTexturePath = "NULL_TEXTURE";
    std::string metallicTexturePath = "NULL_TEXTURE";
    std::string roughnessTexturePath = "NULL_TEXTURE";
    std::string aoTexturePath = "NULL_TEXTURE";

    uint32_t texMask = 0;
    uint32_t metallicChan = 0;
    uint32_t roughnessChan = 0;
    uint32_t aoChan = 0;

    MaterialDto() = default;


    void UpdateTexturePaths(const fs::path& materialsFolder)
    {
        auto fixOne = [&](std::string& p)
        {
            if (IsNullTex(p))
            {
                p = "NULL_TEXTURE";
                return;
            }

            fs::path inPath = fs::path(p);

            // Si ya es absoluto, no tocar
            if (inPath.is_absolute())
            {
                p = NormalizeSlashes(inPath.lexically_normal().string());
                return;
            }

            // Si es relativo (ej. "../Textures/foo.jpg"), resolver desde carpeta Materials
            fs::path resolved = (materialsFolder / inPath).lexically_normal();

            // Si por algún motivo resolved acaba siendo directorio, no machacar: intenta mantener filename original
            // (Esto solo ocurriría si p fuera "../Textures/" sin nombre)
            if (resolved.has_filename() == false || resolved.filename().string().empty())
            {
                // Caso patológico: deja NULL para que se note el error en vez de ruta inválida
                p = "NULL_TEXTURE";
                return;
            }

            p = NormalizeSlashes(resolved.string());
        };

        fixOne(diffuseTexturePath);
        fixOne(normalTexturePath);
        fixOne(metallicTexturePath);
        fixOne(roughnessTexturePath);
        fixOne(aoTexturePath);
        fixOne(emissiveTexturePath);
        fixOne(heightTexturePath);
        fixOne(specularTexturePath);
    }
};

#endif
