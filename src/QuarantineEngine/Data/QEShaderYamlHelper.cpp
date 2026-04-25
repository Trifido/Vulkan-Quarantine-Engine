#include "QEShaderYamlHelper.h"

#include <fstream>
#include <system_error>

#include <Logging/QELogMacros.h>

namespace fs = std::filesystem;

namespace
{
    std::string NormalizeSlashes(std::string path)
    {
        for (char& c : path)
        {
            if (c == '\\')
                c = '/';
        }

        return path;
    }
}

const char* QEShaderYamlHelper::ToString(QEShaderPipelineType value)
{
    switch (value)
    {
    case QEShaderPipelineType::Compute: return "Compute";
    case QEShaderPipelineType::Graphics:
    default: return "Graphics";
    }
}

const char* QEShaderYamlHelper::ToString(QEShaderTopology value)
{
    switch (value)
    {
    case QEShaderTopology::TriangleStrip: return "TriangleStrip";
    case QEShaderTopology::LineList:      return "LineList";
    case QEShaderTopology::LineStrip:     return "LineStrip";
    case QEShaderTopology::PointList:     return "PointList";
    case QEShaderTopology::TriangleList:
    default: return "TriangleList";
    }
}

const char* QEShaderYamlHelper::ToString(QEShaderPolygonMode value)
{
    switch (value)
    {
    case QEShaderPolygonMode::Line:  return "Line";
    case QEShaderPolygonMode::Point: return "Point";
    case QEShaderPolygonMode::Fill:
    default: return "Fill";
    }
}

const char* QEShaderYamlHelper::ToString(QEShaderCullMode value)
{
    switch (value)
    {
    case QEShaderCullMode::None:         return "None";
    case QEShaderCullMode::Front:        return "Front";
    case QEShaderCullMode::FrontAndBack: return "FrontAndBack";
    case QEShaderCullMode::Back:
    default: return "Back";
    }
}

const char* QEShaderYamlHelper::ToString(QEShaderFrontFace value)
{
    switch (value)
    {
    case QEShaderFrontFace::Clockwise: return "Clockwise";
    case QEShaderFrontFace::CounterClockwise:
    default: return "CounterClockwise";
    }
}

const char* QEShaderYamlHelper::ToString(ShadowMappingMode value)
{
    switch (value)
    {
    case ShadowMappingMode::DIRECTIONAL_SHADOW: return "Directional";
    case ShadowMappingMode::OMNI_SHADOW:        return "Omni";
    case ShadowMappingMode::CASCADE_SHADOW:     return "Cascade";
    case ShadowMappingMode::NONE:
    default: return "None";
    }
}

QEShaderPipelineType QEShaderYamlHelper::PipelineTypeFromString(const std::string& value)
{
    if (value == "Compute")
        return QEShaderPipelineType::Compute;

    return QEShaderPipelineType::Graphics;
}

QEShaderTopology QEShaderYamlHelper::TopologyFromString(const std::string& value)
{
    if (value == "TriangleStrip")
        return QEShaderTopology::TriangleStrip;
    if (value == "LineList")
        return QEShaderTopology::LineList;
    if (value == "LineStrip")
        return QEShaderTopology::LineStrip;
    if (value == "PointList")
        return QEShaderTopology::PointList;

    return QEShaderTopology::TriangleList;
}

QEShaderPolygonMode QEShaderYamlHelper::PolygonModeFromString(const std::string& value)
{
    if (value == "Line")
        return QEShaderPolygonMode::Line;
    if (value == "Point")
        return QEShaderPolygonMode::Point;

    return QEShaderPolygonMode::Fill;
}

QEShaderCullMode QEShaderYamlHelper::CullModeFromString(const std::string& value)
{
    if (value == "None")
        return QEShaderCullMode::None;
    if (value == "Front")
        return QEShaderCullMode::Front;
    if (value == "FrontAndBack")
        return QEShaderCullMode::FrontAndBack;

    return QEShaderCullMode::Back;
}

QEShaderFrontFace QEShaderYamlHelper::FrontFaceFromString(const std::string& value)
{
    if (value == "Clockwise")
        return QEShaderFrontFace::Clockwise;

    return QEShaderFrontFace::CounterClockwise;
}

ShadowMappingMode QEShaderYamlHelper::ShadowModeFromString(const std::string& value)
{
    if (value == "Directional")
        return ShadowMappingMode::DIRECTIONAL_SHADOW;
    if (value == "Omni")
        return ShadowMappingMode::OMNI_SHADOW;
    if (value == "Cascade")
        return ShadowMappingMode::CASCADE_SHADOW;

    return ShadowMappingMode::NONE;
}

YAML::Node QEShaderYamlHelper::SerializeShaderAsset(const QEShaderAsset& asset)
{
    YAML::Node root;
    YAML::Node shader;

    shader["Name"] = asset.Name;
    shader["FilePath"] = NormalizeSlashes(asset.FilePath);
    shader["EntryPoint"] = asset.EntryPoint;
    shader["PipelineType"] = ToString(asset.PipelineType);

    YAML::Node stages;
    stages["Vertex"] = NormalizeSlashes(asset.Stages.Vertex);
    stages["Fragment"] = NormalizeSlashes(asset.Stages.Fragment);
    stages["Geometry"] = NormalizeSlashes(asset.Stages.Geometry);
    stages["TessControl"] = NormalizeSlashes(asset.Stages.TessControl);
    stages["TessEvaluation"] = NormalizeSlashes(asset.Stages.TessEvaluation);
    stages["Compute"] = NormalizeSlashes(asset.Stages.Compute);
    stages["Task"] = NormalizeSlashes(asset.Stages.Task);
    stages["Mesh"] = NormalizeSlashes(asset.Stages.Mesh);
    shader["Stages"] = stages;

    YAML::Node graphics;
    graphics["HasVertexData"] = asset.Graphics.HasVertexData;
    graphics["VertexStride"] = asset.Graphics.VertexStride;
    graphics["Topology"] = ToString(asset.Graphics.Topology);
    graphics["PolygonMode"] = ToString(asset.Graphics.PolygonMode);
    graphics["CullMode"] = ToString(asset.Graphics.CullMode);
    graphics["FrontFace"] = ToString(asset.Graphics.FrontFace);
    graphics["DepthTest"] = asset.Graphics.DepthTest;
    graphics["DepthWrite"] = asset.Graphics.DepthWrite;
    graphics["LineWidth"] = asset.Graphics.LineWidth;
    graphics["IsMeshShader"] = asset.Graphics.IsMeshShader;
    shader["Graphics"] = graphics;

    YAML::Node shadow;
    shadow["Enabled"] = asset.Shadow.Enabled;
    shadow["Mode"] = ToString(asset.Shadow.Mode);
    shader["Shadow"] = shadow;

    root["Shader"] = shader;
    return root;
}

bool QEShaderYamlHelper::DeserializeShaderAsset(const YAML::Node& root, QEShaderAsset& asset)
{
    const YAML::Node shader = root["Shader"];
    if (!shader)
        return false;

    asset = QEShaderAsset{};
    asset.Name = shader["Name"] ? shader["Name"].as<std::string>() : "";
    asset.FilePath = shader["FilePath"] ? NormalizeSlashes(shader["FilePath"].as<std::string>()) : "";
    asset.EntryPoint = shader["EntryPoint"] ? shader["EntryPoint"].as<std::string>() : "main";
    asset.PipelineType = shader["PipelineType"]
        ? PipelineTypeFromString(shader["PipelineType"].as<std::string>())
        : QEShaderPipelineType::Graphics;

    if (const YAML::Node stages = shader["Stages"])
    {
        asset.Stages.Vertex = stages["Vertex"] ? NormalizeSlashes(stages["Vertex"].as<std::string>()) : "";
        asset.Stages.Fragment = stages["Fragment"] ? NormalizeSlashes(stages["Fragment"].as<std::string>()) : "";
        asset.Stages.Geometry = stages["Geometry"] ? NormalizeSlashes(stages["Geometry"].as<std::string>()) : "";
        asset.Stages.TessControl = stages["TessControl"] ? NormalizeSlashes(stages["TessControl"].as<std::string>()) : "";
        asset.Stages.TessEvaluation = stages["TessEvaluation"] ? NormalizeSlashes(stages["TessEvaluation"].as<std::string>()) : "";
        asset.Stages.Compute = stages["Compute"] ? NormalizeSlashes(stages["Compute"].as<std::string>()) : "";
        asset.Stages.Task = stages["Task"] ? NormalizeSlashes(stages["Task"].as<std::string>()) : "";
        asset.Stages.Mesh = stages["Mesh"] ? NormalizeSlashes(stages["Mesh"].as<std::string>()) : "";
    }

    if (const YAML::Node graphics = shader["Graphics"])
    {
        asset.Graphics.HasVertexData = graphics["HasVertexData"] ? graphics["HasVertexData"].as<bool>() : true;
        asset.Graphics.VertexStride = graphics["VertexStride"] ? graphics["VertexStride"].as<uint32_t>() : 0u;
        asset.Graphics.Topology = graphics["Topology"]
            ? TopologyFromString(graphics["Topology"].as<std::string>())
            : QEShaderTopology::TriangleList;
        asset.Graphics.PolygonMode = graphics["PolygonMode"]
            ? PolygonModeFromString(graphics["PolygonMode"].as<std::string>())
            : QEShaderPolygonMode::Fill;
        asset.Graphics.CullMode = graphics["CullMode"]
            ? CullModeFromString(graphics["CullMode"].as<std::string>())
            : QEShaderCullMode::Back;
        asset.Graphics.FrontFace = graphics["FrontFace"]
            ? FrontFaceFromString(graphics["FrontFace"].as<std::string>())
            : QEShaderFrontFace::CounterClockwise;
        asset.Graphics.DepthTest = graphics["DepthTest"] ? graphics["DepthTest"].as<bool>() : true;
        asset.Graphics.DepthWrite = graphics["DepthWrite"] ? graphics["DepthWrite"].as<bool>() : true;
        asset.Graphics.LineWidth = graphics["LineWidth"] ? graphics["LineWidth"].as<float>() : 1.0f;
        asset.Graphics.IsMeshShader = graphics["IsMeshShader"] ? graphics["IsMeshShader"].as<bool>() : false;
    }

    if (const YAML::Node shadow = shader["Shadow"])
    {
        asset.Shadow.Enabled = shadow["Enabled"] ? shadow["Enabled"].as<bool>() : false;
        asset.Shadow.Mode = shadow["Mode"]
            ? ShadowModeFromString(shadow["Mode"].as<std::string>())
            : ShadowMappingMode::NONE;
    }

    return true;
}

bool QEShaderYamlHelper::WriteShaderFile(const fs::path& filePath, const QEShaderAsset& asset)
{
    std::error_code ec;
    fs::create_directories(filePath.parent_path(), ec);
    if (ec)
    {
        QE_LOG_ERROR_CAT_F("QEShaderYamlHelper", "Error creating directory for shader asset: {} -> {}", filePath.parent_path().string(), ec.message());
        return false;
    }

    YAML::Emitter out;
    out << SerializeShaderAsset(asset);

    std::ofstream file(filePath, std::ios::out | std::ios::trunc);
    if (!file.is_open())
    {
        QE_LOG_ERROR_CAT_F("QEShaderYamlHelper", "Error opening shader asset file: {}", filePath.string());
        return false;
    }

    file << out.c_str();
    file.close();
    return true;
}

bool QEShaderYamlHelper::ReadShaderFile(const fs::path& filePath, QEShaderAsset& asset)
{
    try
    {
        const YAML::Node root = YAML::LoadFile(filePath.string());
        return DeserializeShaderAsset(root, asset);
    }
    catch (const YAML::BadFile& e)
    {
        QE_LOG_ERROR_CAT_F("QEShaderYamlHelper", "The shader asset could not be opened: {} ({})", filePath.string(), e.what());
        return false;
    }
    catch (const YAML::ParserException& e)
    {
        QE_LOG_ERROR_CAT_F("QEShaderYamlHelper", "Invalid shader YAML: {} ({})", filePath.string(), e.what());
        return false;
    }
}
