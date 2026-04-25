#pragma once

#ifndef QE_SHADER_ASSET_LOADER_H
#define QE_SHADER_ASSET_LOADER_H

#include <filesystem>
#include <memory>
#include <string>

#include <GraphicsPipelineData.h>
#include <QEShaderAsset.h>

class ShaderModule;

namespace fs = std::filesystem;

class QEShaderAssetLoader
{
public:
    static bool IsShaderAssetPath(const fs::path& path);
    static std::shared_ptr<ShaderModule> LoadShaderAsset(const fs::path& shaderAssetPath, bool forceReload = false);
    static std::string BuildShaderId(const fs::path& shaderAssetPath);

private:
    static GraphicsPipelineData BuildGraphicsPipelineData(const QEShaderAsset& asset);
    static fs::path ResolveStagePath(const std::string& stagePath, const fs::path& shaderAssetPath);

    static VkPrimitiveTopology ToVkTopology(QEShaderTopology topology);
    static VkPolygonMode ToVkPolygonMode(QEShaderPolygonMode polygonMode);
    static VkCullModeFlags ToVkCullMode(QEShaderCullMode cullMode);
    static VkFrontFace ToVkFrontFace(QEShaderFrontFace frontFace);
};

#endif
