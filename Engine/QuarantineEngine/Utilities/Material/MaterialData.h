#pragma once

#ifndef MATERIAL_DATA_H
#define MATERIAL_DATA_H

#include <assimp/material.h>
#include <glm/ext/vector_float3.hpp>
#include <CustomTexture.h>
#include <TextureManager.h>
#include <assimp/scene.h>
#include <set>

class MaterialData
{
private:
    std::set<std::string> currentTextures;
    std::shared_ptr<CustomTexture> emptyTexture = nullptr;
    std::shared_ptr <std::vector<std::shared_ptr<CustomTexture>>> texture_vector = nullptr;
    TextureManager* textureManager;
    std::string fileExtension;
    std::string texturePath;
    int numTextures;

public:
    float Opacity;
    float BumpScaling;
    float Shininess;
    float Reflectivity;
    float Shininess_Strength;
    float Refractivity;

    glm::vec3 Diffuse;
    glm::vec3 Ambient;
    glm::vec3 Specular;
    glm::vec3 Emissive;
    glm::vec3 Transparent;
    glm::vec3 Reflective;

    static const int TOTAL_NUM_TEXTURES = 5;
    std::shared_ptr<CustomTexture> diffuseTexture = nullptr;
    std::shared_ptr<CustomTexture> normalTexture = nullptr;
    std::shared_ptr<CustomTexture> specularTexture = nullptr;
    std::shared_ptr<CustomTexture> emissiveTexture = nullptr;
    std::shared_ptr<CustomTexture> heightTexture = nullptr;

    int idxDiffuse;
    int idxNormal;
    int idxSpecular;
    int idxHeight;
    int idxEmissive;

private:
    void AddTexture(std::shared_ptr<CustomTexture> texture);
    std::shared_ptr<CustomTexture> findTextureByType(TEXTURE_TYPE newtype);
    std::string GetTexture(const aiScene* scene, aiMaterial* mat, aiTextureType type, TEXTURE_TYPE textureType);
    void fillEmptyTextures();

public:
    MaterialData();
    void ImportAssimpMaterial(aiMaterial* material);
    void ImportAssimpTexture(const aiScene* scene, aiMaterial* material, std::string fileExtension, std::string texturePath);
};

#endif // !MATERIAL_DATA_H


