#pragma once
#ifndef MATERIAL_DATA_H
#define MATERIAL_DATA_H

#include <assimp/material.h>
#include <assimp/scene.h>

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <SynchronizationModule.h>
#include <CustomTexture.h>
#include <TextureManager.h>
#include <ShaderModule.h>
#include <UBO.h>

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <set>
#include <unordered_map>
#include <functional>
#include <algorithm>
#include <stdexcept>
#include <memory>

struct FieldInfo
{
    size_t offset = 0;
    size_t size = 0;
};

using WriteFn = std::function<void(uint8_t* dst, size_t offset)>;

class MaterialData
{
private:
    DeviceModule* deviceModule = nullptr;
    TextureManager* textureManager = nullptr;

    std::set<std::string> currentTextures;
    std::shared_ptr<CustomTexture> emptyTexture = nullptr;

    std::string fileExtension;
    std::string texturePath;

    int numTextures = 0;

    // Nuevo buffer “plano” para UBO
    std::vector<uint8_t> materialBuffer;

    std::unordered_map<std::string, FieldInfo> materialFields;
    std::unordered_map<std::string, WriteFn> writers;

    bool isModified[MAX_FRAMES_IN_FLIGHT] = {};

public:
    // Scalars
    float Opacity = 1.0f;
    float BumpScaling = 1.0f;
    float Shininess = 32.0f;
    float Reflectivity = 0.0f;
    float Shininess_Strength = 0.0f;
    float Refractivity = 0.0f;

    // Colors
    glm::vec4 Diffuse = glm::vec4(1.0f);
    glm::vec4 Ambient = glm::vec4(0.1f);
    glm::vec4 Specular = glm::vec4(1.0f);
    glm::vec4 Emissive = glm::vec4(0.0f);
    glm::vec4 Transparent = glm::vec4(1.0f);
    glm::vec4 Reflective = glm::vec4(0.0f);

    static const int TOTAL_NUM_TEXTURES = 5;

    std::unordered_map<TEXTURE_TYPE, std::shared_ptr<CustomTexture>> Textures;
    std::unordered_map<TEXTURE_TYPE, int> IDTextures;

    std::shared_ptr<UniformBufferObject> materialUBO = nullptr;
    VkDeviceSize materialUniformSize = 0;

    std::shared_ptr<std::vector<std::shared_ptr<CustomTexture>>> texture_vector = nullptr;

private:
    static std::string NormalizeMaterialMemberName(std::string name);
    std::unordered_map<std::string, WriteFn> BuildMaterialWriters();

    void WriteToMaterialBufferAt(const void* data, size_t offset, size_t size);
    void UpdateMaterialDataRaw(const std::string& fieldName, const void* src, size_t size);

    std::shared_ptr<CustomTexture> findTextureByType(TEXTURE_TYPE newtype);
    std::string GetTexture(const aiScene* scene, aiMaterial* mat, aiTextureType type, TEXTURE_TYPE textureType);
    void fillEmptyTextures();

    static glm::vec4 ToVec4(const aiColor4D& c)
    {
        return glm::vec4(c.r, c.g, c.b, c.a);
    }

public:
    MaterialData();

    void AddTexture(const std::string& textureName, const std::shared_ptr<CustomTexture>& texture);

    void ImportAssimpMaterial(aiMaterial* material);
    void ImportAssimpTexture(const aiScene* scene, aiMaterial* material, std::string fileExtension, std::string texturePath);

    void InitializeUBOMaterial(std::shared_ptr<ShaderModule> shader_ptr);
    void UpdateUBOMaterial();

    void CleanMaterialUBO();
    void CleanLastResources();

    void SetMaterialField(const std::string& nameField, float value);
    void SetMaterialField(const std::string& nameField, glm::vec3 value);
    void SetMaterialField(const std::string& nameField, int value);
};

#endif // MATERIAL_DATA_H
