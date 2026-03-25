#pragma once
#ifndef QE_MATERIAL_YAML_HELPER_H
#define QE_MATERIAL_YAML_HELPER_H

#include <filesystem>
#include <yaml-cpp/yaml.h>
#include "MaterialDto.h"

class QEMaterialYamlHelper
{
public:
    static YAML::Node SerializeMaterialDto(const MaterialDto& dto);
    static bool DeserializeMaterialDto(const YAML::Node& root, MaterialDto& dto);

    static bool WriteMaterialFile(const std::filesystem::path& filePath, const MaterialDto& dto);
    static bool ReadMaterialFile(const std::filesystem::path& filePath, MaterialDto& dto);

private:
    static YAML::Node SerializeVec4(const glm::vec4& value);
    static glm::vec4 DeserializeVec4(const YAML::Node& node, const glm::vec4& defaultValue);

    static std::string NormalizeYamlTexturePath(const std::string& path);
};
#endif
