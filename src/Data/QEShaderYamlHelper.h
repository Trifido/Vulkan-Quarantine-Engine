#pragma once

#ifndef QE_SHADER_YAML_HELPER_H
#define QE_SHADER_YAML_HELPER_H

#include <filesystem>

#include <yaml-cpp/yaml.h>

#include "QEShaderAsset.h"

class QEShaderYamlHelper
{
public:
    static YAML::Node SerializeShaderAsset(const QEShaderAsset& asset);
    static bool DeserializeShaderAsset(const YAML::Node& root, QEShaderAsset& asset);

    static bool WriteShaderFile(const std::filesystem::path& filePath, const QEShaderAsset& asset);
    static bool ReadShaderFile(const std::filesystem::path& filePath, QEShaderAsset& asset);

private:
    static const char* ToString(QEShaderPipelineType value);
    static const char* ToString(QEShaderTopology value);
    static const char* ToString(QEShaderPolygonMode value);
    static const char* ToString(QEShaderCullMode value);
    static const char* ToString(QEShaderFrontFace value);
    static const char* ToString(ShadowMappingMode value);

    static QEShaderPipelineType PipelineTypeFromString(const std::string& value);
    static QEShaderTopology TopologyFromString(const std::string& value);
    static QEShaderPolygonMode PolygonModeFromString(const std::string& value);
    static QEShaderCullMode CullModeFromString(const std::string& value);
    static QEShaderFrontFace FrontFaceFromString(const std::string& value);
    static ShadowMappingMode ShadowModeFromString(const std::string& value);
};

#endif
