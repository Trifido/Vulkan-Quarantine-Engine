#pragma once

#include <cstdint>
#include <string>
#include <string_view>

#include <yaml-cpp/yaml.h>

namespace QE::AssetSchema
{
inline constexpr uint32_t LegacyVersion = 0;
inline constexpr uint32_t SceneVersion = 1;
inline constexpr uint32_t MaterialVersion = 1;
inline constexpr uint32_t ShaderVersion = 1;
inline constexpr uint32_t AnimationGraphVersion = 1;
inline constexpr uint32_t EditorSceneStateVersion = 1;

inline void Stamp(YAML::Node& root, std::string_view assetType, uint32_t version)
{
    root["AssetType"] = std::string(assetType);
    root["SchemaVersion"] = version;
}

inline bool ValidateReadable(
    const YAML::Node& root,
    std::string_view expectedAssetType,
    uint32_t currentVersion,
    uint32_t* sourceVersion = nullptr,
    std::string* errorMessage = nullptr)
{
    try
    {
        const YAML::Node typeNode = root["AssetType"];
        const YAML::Node versionNode = root["SchemaVersion"];
        if (static_cast<bool>(typeNode) != static_cast<bool>(versionNode))
        {
            if (errorMessage != nullptr)
                *errorMessage = "Schema metadata must contain both AssetType and SchemaVersion";
            return false;
        }

        if (typeNode && typeNode.as<std::string>() != expectedAssetType)
        {
            if (errorMessage != nullptr)
                *errorMessage = "Unexpected asset type '" + typeNode.as<std::string>() + "'";
            return false;
        }

        const uint32_t version = versionNode
            ? versionNode.as<uint32_t>()
            : LegacyVersion;

        if (sourceVersion != nullptr)
            *sourceVersion = version;

        if (version > currentVersion)
        {
            if (errorMessage != nullptr)
            {
                *errorMessage = "Asset schema version " + std::to_string(version) +
                    " is newer than supported version " + std::to_string(currentVersion);
            }
            return false;
        }

        return true;
    }
    catch (const YAML::Exception& error)
    {
        if (errorMessage != nullptr)
            *errorMessage = "Invalid schema metadata: " + std::string(error.what());
        return false;
    }
}
} // namespace QE::AssetSchema
