#include "QEShaderAssetLoader.h"

#include <algorithm>
#include <stdexcept>

#include <Logging/QELogMacros.h>
#include <QEProjectManager.h>
#include <QEShaderYamlHelper.h>
#include <ShaderManager.h>
#include <ShaderModule.h>
#include <Vertex.h>

namespace
{
    std::string ToLower(std::string value)
    {
        std::transform(
            value.begin(),
            value.end(),
            value.begin(),
            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

        return value;
    }
}

bool QEShaderAssetLoader::IsShaderAssetPath(const fs::path& path)
{
    return ToLower(path.extension().string()) == ".qeshader";
}

std::shared_ptr<ShaderModule> QEShaderAssetLoader::LoadShaderAsset(const fs::path& shaderAssetPath)
{
    const fs::path resolvedAssetPath = QEProjectManager::ResolveProjectPath(shaderAssetPath);

    QEShaderAsset asset{};
    if (!QEShaderYamlHelper::ReadShaderFile(resolvedAssetPath, asset))
    {
        QE_LOG_ERROR_CAT_F("QEShaderAssetLoader", "Failed to read shader asset {}", resolvedAssetPath.string());
        return nullptr;
    }

    const std::string shaderId = BuildShaderId(resolvedAssetPath);
    auto shaderManager = ShaderManager::getInstance();

    if (auto existingShader = shaderManager->GetShader(shaderId))
        return existingShader;

    const GraphicsPipelineData pipelineData = BuildGraphicsPipelineData(asset);

    std::shared_ptr<ShaderModule> shaderModule;

    try
    {
        if (!asset.Stages.TessControl.empty() || !asset.Stages.TessEvaluation.empty())
        {
            throw std::runtime_error("Tessellation shaders are not wired into ShaderModule yet.");
        }

        if (asset.PipelineType == QEShaderPipelineType::Compute)
        {
            const fs::path computePath = ResolveStagePath(asset.Stages.Compute, resolvedAssetPath);
            if (computePath.empty())
                throw std::runtime_error("Compute shader asset has no Compute stage.");

            shaderModule = std::make_shared<ShaderModule>(
                ShaderModule(shaderId, computePath.string(), pipelineData));
        }
        else if (!asset.Stages.Mesh.empty())
        {
            const fs::path taskPath = ResolveStagePath(asset.Stages.Task, resolvedAssetPath);
            const fs::path meshPath = ResolveStagePath(asset.Stages.Mesh, resolvedAssetPath);
            const fs::path fragmentPath = ResolveStagePath(asset.Stages.Fragment, resolvedAssetPath);

            if (meshPath.empty() || fragmentPath.empty())
                throw std::runtime_error("Mesh shader asset requires Mesh and Fragment stages.");

            shaderModule = std::make_shared<ShaderModule>(
                ShaderModule(shaderId, taskPath.string(), meshPath.string(), fragmentPath.string(), pipelineData));
        }
        else if (!asset.Stages.Geometry.empty())
        {
            const fs::path vertexPath = ResolveStagePath(asset.Stages.Vertex, resolvedAssetPath);
            const fs::path geometryPath = ResolveStagePath(asset.Stages.Geometry, resolvedAssetPath);
            const fs::path fragmentPath = ResolveStagePath(asset.Stages.Fragment, resolvedAssetPath);

            if (vertexPath.empty() || geometryPath.empty() || fragmentPath.empty())
                throw std::runtime_error("Geometry shader asset requires Vertex, Geometry and Fragment stages.");

            shaderModule = std::make_shared<ShaderModule>(
                ShaderModule(shaderId, vertexPath.string(), geometryPath.string(), fragmentPath.string(), pipelineData));
        }
        else if (!asset.Stages.Vertex.empty() && !asset.Stages.Fragment.empty())
        {
            const fs::path vertexPath = ResolveStagePath(asset.Stages.Vertex, resolvedAssetPath);
            const fs::path fragmentPath = ResolveStagePath(asset.Stages.Fragment, resolvedAssetPath);

            shaderModule = std::make_shared<ShaderModule>(
                ShaderModule(shaderId, vertexPath.string(), fragmentPath.string(), pipelineData));
        }
        else if (!asset.Stages.Vertex.empty() && asset.Shadow.Enabled)
        {
            const fs::path vertexPath = ResolveStagePath(asset.Stages.Vertex, resolvedAssetPath);

            shaderModule = std::make_shared<ShaderModule>(
                ShaderModule(shaderId, vertexPath.string(), pipelineData));
        }
        else
        {
            throw std::runtime_error("Unsupported shader stage combination in .qeshader.");
        }
    }
    catch (const std::exception& e)
    {
        QE_LOG_ERROR_CAT_F(
            "QEShaderAssetLoader",
            "Failed to build shader asset {} ({})",
            resolvedAssetPath.string(),
            e.what());
        return nullptr;
    }

    shaderManager->UpsertShader(shaderModule);
    return shaderModule;
}

std::string QEShaderAssetLoader::BuildShaderId(const fs::path& shaderAssetPath)
{
    std::string relativePath = QEProjectManager::ToProjectRelativePath(shaderAssetPath);
    for (char& c : relativePath)
    {
        if (c == '\\' || c == '/' || c == '.' || c == ' ' || c == ':')
            c = '_';
    }

    return "qeshader_" + relativePath;
}

GraphicsPipelineData QEShaderAssetLoader::BuildGraphicsPipelineData(const QEShaderAsset& asset)
{
    GraphicsPipelineData pipelineData{};
    pipelineData.polygonMode = ToVkPolygonMode(asset.Graphics.PolygonMode);
    pipelineData.topology = ToVkTopology(asset.Graphics.Topology);
    pipelineData.cullMode = ToVkCullMode(asset.Graphics.CullMode);
    pipelineData.frontFace = ToVkFrontFace(asset.Graphics.FrontFace);
    pipelineData.lineWidth = asset.Graphics.LineWidth;
    pipelineData.HasVertexData = asset.Graphics.HasVertexData;
    pipelineData.IsMeshShader = asset.Graphics.IsMeshShader;
    pipelineData.DepthTestEnabled = asset.Graphics.DepthTest;
    pipelineData.DepthWriteEnabled = asset.Graphics.DepthWrite;
    pipelineData.shadowMode = asset.Shadow.Enabled ? asset.Shadow.Mode : ShadowMappingMode::NONE;
    pipelineData.vertexBufferStride =
        asset.Graphics.VertexStride > 0 ? asset.Graphics.VertexStride : static_cast<uint32_t>(sizeof(Vertex));
    return pipelineData;
}

fs::path QEShaderAssetLoader::ResolveStagePath(const std::string& stagePath, const fs::path& shaderAssetPath)
{
    if (stagePath.empty())
        return {};

    fs::path path(stagePath);
    if (path.is_absolute())
        return path.lexically_normal();

    const fs::path projectResolvedPath = QEProjectManager::ResolveProjectPath(path);
    if (fs::exists(projectResolvedPath))
        return projectResolvedPath.lexically_normal();

    return (shaderAssetPath.parent_path() / path).lexically_normal();
}

VkPrimitiveTopology QEShaderAssetLoader::ToVkTopology(QEShaderTopology topology)
{
    switch (topology)
    {
    case QEShaderTopology::TriangleStrip: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
    case QEShaderTopology::LineList:      return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
    case QEShaderTopology::LineStrip:     return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
    case QEShaderTopology::PointList:     return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
    case QEShaderTopology::TriangleList:
    default: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    }
}

VkPolygonMode QEShaderAssetLoader::ToVkPolygonMode(QEShaderPolygonMode polygonMode)
{
    switch (polygonMode)
    {
    case QEShaderPolygonMode::Line:  return VK_POLYGON_MODE_LINE;
    case QEShaderPolygonMode::Point: return VK_POLYGON_MODE_POINT;
    case QEShaderPolygonMode::Fill:
    default: return VK_POLYGON_MODE_FILL;
    }
}

VkCullModeFlags QEShaderAssetLoader::ToVkCullMode(QEShaderCullMode cullMode)
{
    switch (cullMode)
    {
    case QEShaderCullMode::None:         return VK_CULL_MODE_NONE;
    case QEShaderCullMode::Front:        return VK_CULL_MODE_FRONT_BIT;
    case QEShaderCullMode::FrontAndBack: return VK_CULL_MODE_FRONT_AND_BACK;
    case QEShaderCullMode::Back:
    default: return VK_CULL_MODE_BACK_BIT;
    }
}

VkFrontFace QEShaderAssetLoader::ToVkFrontFace(QEShaderFrontFace frontFace)
{
    switch (frontFace)
    {
    case QEShaderFrontFace::Clockwise: return VK_FRONT_FACE_CLOCKWISE;
    case QEShaderFrontFace::CounterClockwise:
    default: return VK_FRONT_FACE_COUNTER_CLOCKWISE;
    }
}
