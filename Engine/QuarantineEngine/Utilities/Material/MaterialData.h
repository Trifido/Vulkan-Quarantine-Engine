#pragma once

#ifndef MATERIAL_DATA_H
#define MATERIAL_DATA_H

#include <assimp/material.h>
#include <glm/ext/vector_float3.hpp>
#include <CustomTexture.h>
#include <TextureManager.h>
#include <assimp/scene.h>
#include <set>
#include <ShaderModule.h>
#include <UBO.h>

class MaterialData
{
private:
    DeviceModule* deviceModule;
    std::set<std::string> currentTextures;
    std::shared_ptr<CustomTexture> emptyTexture = nullptr;
    TextureManager* textureManager;
    std::string fileExtension;
    std::string texturePath;
    int numTextures;
    char* materialbuffer;
    std::unordered_map < std::string, std::pair<size_t, size_t> > materialFields;
    bool isModified[2] = { true, true };
    void* auxiliarBuffer[2];

public:
    float Opacity;
    float BumpScaling;
    float Shininess;
    float Reflectivity;
    float Shininess_Strength;
    float Refractivity;

    glm::vec4 Diffuse;
    glm::vec4 Ambient;
    glm::vec4 Specular;
    glm::vec4 Emissive;
    glm::vec4 Transparent;
    glm::vec4 Reflective;

    static const int TOTAL_NUM_TEXTURES = 5;
    std::unordered_map<TEXTURE_TYPE, std::shared_ptr<CustomTexture>> Textures;
    std::unordered_map<TEXTURE_TYPE, int> IDTextures;

    std::shared_ptr<UniformBufferObject> materialUBO = nullptr;
    VkDeviceSize materialUniformSize = 0;
    std::shared_ptr <std::vector<std::shared_ptr<CustomTexture>>> texture_vector = nullptr;

private:
    std::shared_ptr<CustomTexture> findTextureByType(TEXTURE_TYPE newtype);
    std::string GetTexture(const aiScene* scene, aiMaterial* mat, aiTextureType type, TEXTURE_TYPE textureType);
    void fillEmptyTextures();
    void WriteToMaterialBuffer(char* bufferdata, size_t& position, const size_t& sizeToCopy);
    void UpdateMaterialBuffer(char* bufferdata, size_t &position, const size_t& sizeToCopy);
    void UpdateMaterialData(std::string materialField, char* value);
public:
    MaterialData();
    void AddTexture(std::string textureName, std::shared_ptr<CustomTexture> texture);
    void ImportAssimpMaterial(aiMaterial* material);
    void ImportAssimpTexture(const aiScene* scene, aiMaterial* material, std::string fileExtension, std::string texturePath);
    void InitializeUBOMaterial(std::shared_ptr<ShaderModule> shader_ptr);
    void UpdateUBOMaterial();
    void CleanMaterialUBO();
    void CleanLastResources();
    void SetMaterialField(std::string nameField, float value);
    void SetMaterialField(std::string nameField, glm::vec3 value);
    void SetMaterialField(std::string nameField, int value);
};

#endif // !MATERIAL_DATA_H


