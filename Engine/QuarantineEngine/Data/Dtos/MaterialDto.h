#pragma once
#ifndef MATERIAL_DTO_H
#define MATERIAL_DTO_H

#include <glm/glm.hpp>
#include <string>
#include <filesystem>

namespace fs = std::filesystem;

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

    MaterialDto() = default;

    void UpdateTexturePaths(fs::path filepath)
    {
        if (diffuseTexturePath != "NULL_TEXTURE")
        {
            diffuseTexturePath = (filepath / diffuseTexturePath).string();
        }
        if (normalTexturePath != "NULL_TEXTURE")
        {
            normalTexturePath = (filepath / normalTexturePath).string();
        }
        if (specularTexturePath != "NULL_TEXTURE")
        {
            specularTexturePath = (filepath / specularTexturePath).string();
        }
        if (emissiveTexturePath != "NULL_TEXTURE")
        {
            emissiveTexturePath = (filepath / emissiveTexturePath).string();
        }
        if (heightTexturePath != "NULL_TEXTURE")
        {
            heightTexturePath = (filepath / heightTexturePath).string();
        }
    }
};

#endif
