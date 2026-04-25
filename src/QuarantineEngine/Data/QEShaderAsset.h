#pragma once

#ifndef QE_SHADER_ASSET_H
#define QE_SHADER_ASSET_H

#include <cstdint>
#include <string>

#include <ShadowMappingMode.h>

enum class QEShaderPipelineType
{
    Graphics = 0,
    Compute = 1
};

enum class QEShaderTopology
{
    TriangleList = 0,
    TriangleStrip = 1,
    LineList = 2,
    LineStrip = 3,
    PointList = 4
};

enum class QEShaderPolygonMode
{
    Fill = 0,
    Line = 1,
    Point = 2
};

enum class QEShaderCullMode
{
    None = 0,
    Front = 1,
    Back = 2,
    FrontAndBack = 3
};

enum class QEShaderFrontFace
{
    CounterClockwise = 0,
    Clockwise = 1
};

struct QEShaderStagePaths
{
    std::string Vertex;
    std::string Fragment;
    std::string Geometry;
    std::string TessControl;
    std::string TessEvaluation;
    std::string Compute;
    std::string Task;
    std::string Mesh;
};

struct QEShaderGraphicsState
{
    bool HasVertexData = true;
    uint32_t VertexStride = 0;
    QEShaderTopology Topology = QEShaderTopology::TriangleList;
    QEShaderPolygonMode PolygonMode = QEShaderPolygonMode::Fill;
    QEShaderCullMode CullMode = QEShaderCullMode::Back;
    QEShaderFrontFace FrontFace = QEShaderFrontFace::CounterClockwise;
    bool DepthTest = true;
    bool DepthWrite = true;
    float LineWidth = 1.0f;
    bool IsMeshShader = false;
};

struct QEShaderShadowState
{
    bool Enabled = false;
    ShadowMappingMode Mode = ShadowMappingMode::NONE;
};

struct QEShaderAsset
{
    std::string Name;
    std::string FilePath;
    std::string EntryPoint = "main";
    QEShaderPipelineType PipelineType = QEShaderPipelineType::Graphics;
    QEShaderStagePaths Stages;
    QEShaderGraphicsState Graphics;
    QEShaderShadowState Shadow;
};



namespace QE
{
    using ::QEShaderPipelineType;
    using ::QEShaderTopology;
    using ::QEShaderPolygonMode;
    using ::QEShaderCullMode;
    using ::QEShaderFrontFace;
    using ::QEShaderStagePaths;
    using ::QEShaderGraphicsState;
    using ::QEShaderShadowState;
    using ::QEShaderAsset;
} // namespace QE
// QE namespace aliases
#endif
