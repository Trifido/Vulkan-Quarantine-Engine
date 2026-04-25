#include "ShaderEditorPanel.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <vector>

#include <imgui.h>

#include <Editor/Core/EditorContext.h>
#include <Logging/QELogMacros.h>
#include <MaterialManager.h>
#include <QEProjectManager.h>
#include <QEShaderYamlHelper.h>

namespace
{
    const char* GetPipelineTypeLabel(QEShaderPipelineType value)
    {
        switch (value)
        {
        case QEShaderPipelineType::Compute: return "Compute";
        case QEShaderPipelineType::Graphics:
        default: return "Graphics";
        }
    }

    const char* GetTopologyLabel(QEShaderTopology value)
    {
        switch (value)
        {
        case QEShaderTopology::TriangleStrip: return "Triangle Strip";
        case QEShaderTopology::LineList:      return "Line List";
        case QEShaderTopology::LineStrip:     return "Line Strip";
        case QEShaderTopology::PointList:     return "Point List";
        case QEShaderTopology::TriangleList:
        default: return "Triangle List";
        }
    }

    const char* GetPolygonModeLabel(QEShaderPolygonMode value)
    {
        switch (value)
        {
        case QEShaderPolygonMode::Line:  return "Line";
        case QEShaderPolygonMode::Point: return "Point";
        case QEShaderPolygonMode::Fill:
        default: return "Fill";
        }
    }

    const char* GetCullModeLabel(QEShaderCullMode value)
    {
        switch (value)
        {
        case QEShaderCullMode::None:         return "None";
        case QEShaderCullMode::Front:        return "Front";
        case QEShaderCullMode::FrontAndBack: return "Front And Back";
        case QEShaderCullMode::Back:
        default: return "Back";
        }
    }

    const char* GetFrontFaceLabel(QEShaderFrontFace value)
    {
        switch (value)
        {
        case QEShaderFrontFace::Clockwise: return "Clockwise";
        case QEShaderFrontFace::CounterClockwise:
        default: return "Counter Clockwise";
        }
    }

    const char* GetShadowModeLabel(ShadowMappingMode value)
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

    bool IsSpirvPath(const std::filesystem::path& path)
    {
        std::string ext = path.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        return ext == ".spv";
    }
}

ShaderEditorPanel::ShaderEditorPanel(EditorContext* editorContext)
    : editorContext(editorContext)
{
}

bool ShaderEditorPanel::OpenShaderFromFile(const std::filesystem::path& shaderPath)
{
    QEShaderAsset asset{};
    if (!QEShaderYamlHelper::ReadShaderFile(shaderPath, asset))
        return false;

    _shaderPath = shaderPath;
    _asset = asset;
    _isOpen = true;
    _isDirty = false;

    if (editorContext)
        editorContext->ShowShaderEditor = true;

    return true;
}

void ShaderEditorPanel::Draw()
{
    if (!editorContext || !editorContext->ShowShaderEditor)
        return;

    bool open = editorContext->ShowShaderEditor;
    if (!ImGui::Begin("Shader Editor", &open))
    {
        editorContext->ShowShaderEditor = open;
        ImGui::End();
        return;
    }

    editorContext->ShowShaderEditor = open;

    if (_shaderPath.empty())
    {
        ImGui::TextUnformatted("No shader asset selected.");
        ImGui::End();
        return;
    }

    DrawToolbar();
    ImGui::Separator();
    DrawGeneralSection();
    DrawStagesSection();
    DrawGraphicsSection();
    DrawShadowSection();

    ImGui::End();
}

void ShaderEditorPanel::DrawToolbar()
{
    ImGui::Text("Shader: %s", _asset.Name.c_str());
    ImGui::TextWrapped("%s", _shaderPath.string().c_str());

    if (ImGui::Button("Save"))
    {
        Save();
    }

    ImGui::SameLine();
    if (_isDirty)
        ImGui::TextUnformatted("Modified");
}

void ShaderEditorPanel::DrawGeneralSection()
{
    ImGui::SeparatorText("General");

    std::array<char, 256> nameBuffer{};
    std::strncpy(nameBuffer.data(), _asset.Name.c_str(), nameBuffer.size() - 1);
    if (ImGui::InputText("Name", nameBuffer.data(), nameBuffer.size()))
    {
        _asset.Name = nameBuffer.data();
        _isDirty = true;
    }

    std::array<char, 128> entryPointBuffer{};
    std::strncpy(entryPointBuffer.data(), _asset.EntryPoint.c_str(), entryPointBuffer.size() - 1);
    if (ImGui::InputText("Entry Point", entryPointBuffer.data(), entryPointBuffer.size()))
    {
        _asset.EntryPoint = entryPointBuffer.data();
        _isDirty = true;
    }

    if (ImGui::BeginCombo("Pipeline Type", GetPipelineTypeLabel(_asset.PipelineType)))
    {
        const QEShaderPipelineType types[] =
        {
            QEShaderPipelineType::Graphics,
            QEShaderPipelineType::Compute
        };

        for (const auto type : types)
        {
            const bool selected = (_asset.PipelineType == type);
            if (ImGui::Selectable(GetPipelineTypeLabel(type), selected))
            {
                _asset.PipelineType = type;
                _isDirty = true;
            }

            if (selected)
                ImGui::SetItemDefaultFocus();
        }

        ImGui::EndCombo();
    }
}

void ShaderEditorPanel::DrawStagesSection()
{
    ImGui::SeparatorText("Stages");

    DrawStagePathField("Vertex", _asset.Stages.Vertex);
    DrawStagePathField("Fragment", _asset.Stages.Fragment);
    DrawStagePathField("Geometry", _asset.Stages.Geometry);
    DrawStagePathField("Tess Control", _asset.Stages.TessControl);
    DrawStagePathField("Tess Eval", _asset.Stages.TessEvaluation);
    DrawStagePathField("Compute", _asset.Stages.Compute);
    DrawStagePathField("Task", _asset.Stages.Task);
    DrawStagePathField("Mesh", _asset.Stages.Mesh);
}

void ShaderEditorPanel::DrawGraphicsSection()
{
    if (_asset.PipelineType != QEShaderPipelineType::Graphics)
        return;

    ImGui::SeparatorText("Graphics");

    if (ImGui::Checkbox("Has Vertex Data", &_asset.Graphics.HasVertexData))
        _isDirty = true;

    int vertexStride = static_cast<int>(_asset.Graphics.VertexStride);
    if (ImGui::DragInt("Vertex Stride", &vertexStride, 1.0f, 0, 4096))
    {
        _asset.Graphics.VertexStride = static_cast<uint32_t>(std::max(0, vertexStride));
        _isDirty = true;
    }

    if (ImGui::BeginCombo("Topology", GetTopologyLabel(_asset.Graphics.Topology)))
    {
        const QEShaderTopology topologies[] =
        {
            QEShaderTopology::TriangleList,
            QEShaderTopology::TriangleStrip,
            QEShaderTopology::LineList,
            QEShaderTopology::LineStrip,
            QEShaderTopology::PointList
        };

        for (const auto topology : topologies)
        {
            const bool selected = (_asset.Graphics.Topology == topology);
            if (ImGui::Selectable(GetTopologyLabel(topology), selected))
            {
                _asset.Graphics.Topology = topology;
                _isDirty = true;
            }

            if (selected)
                ImGui::SetItemDefaultFocus();
        }

        ImGui::EndCombo();
    }

    if (ImGui::BeginCombo("Polygon Mode", GetPolygonModeLabel(_asset.Graphics.PolygonMode)))
    {
        const QEShaderPolygonMode modes[] =
        {
            QEShaderPolygonMode::Fill,
            QEShaderPolygonMode::Line,
            QEShaderPolygonMode::Point
        };

        for (const auto mode : modes)
        {
            const bool selected = (_asset.Graphics.PolygonMode == mode);
            if (ImGui::Selectable(GetPolygonModeLabel(mode), selected))
            {
                _asset.Graphics.PolygonMode = mode;
                _isDirty = true;
            }

            if (selected)
                ImGui::SetItemDefaultFocus();
        }

        ImGui::EndCombo();
    }

    if (ImGui::DragFloat("Line Width", &_asset.Graphics.LineWidth, 0.1f, 0.1f, 16.0f))
        _isDirty = true;

    if (ImGui::BeginCombo("Cull Mode", GetCullModeLabel(_asset.Graphics.CullMode)))
    {
        const QEShaderCullMode modes[] =
        {
            QEShaderCullMode::None,
            QEShaderCullMode::Front,
            QEShaderCullMode::Back,
            QEShaderCullMode::FrontAndBack
        };

        for (const auto mode : modes)
        {
            const bool selected = (_asset.Graphics.CullMode == mode);
            if (ImGui::Selectable(GetCullModeLabel(mode), selected))
            {
                _asset.Graphics.CullMode = mode;
                _isDirty = true;
            }

            if (selected)
                ImGui::SetItemDefaultFocus();
        }

        ImGui::EndCombo();
    }

    if (ImGui::BeginCombo("Front Face", GetFrontFaceLabel(_asset.Graphics.FrontFace)))
    {
        const QEShaderFrontFace modes[] =
        {
            QEShaderFrontFace::CounterClockwise,
            QEShaderFrontFace::Clockwise
        };

        for (const auto mode : modes)
        {
            const bool selected = (_asset.Graphics.FrontFace == mode);
            if (ImGui::Selectable(GetFrontFaceLabel(mode), selected))
            {
                _asset.Graphics.FrontFace = mode;
                _isDirty = true;
            }

            if (selected)
                ImGui::SetItemDefaultFocus();
        }

        ImGui::EndCombo();
    }

    if (ImGui::Checkbox("Depth Test", &_asset.Graphics.DepthTest))
        _isDirty = true;

    if (ImGui::Checkbox("Depth Write", &_asset.Graphics.DepthWrite))
        _isDirty = true;

    if (ImGui::Checkbox("Is Mesh Shader", &_asset.Graphics.IsMeshShader))
        _isDirty = true;
}

void ShaderEditorPanel::DrawShadowSection()
{
    ImGui::SeparatorText("Shadow");

    if (ImGui::Checkbox("Shadow Enabled", &_asset.Shadow.Enabled))
        _isDirty = true;

    if (ImGui::BeginCombo("Shadow Mode", GetShadowModeLabel(_asset.Shadow.Mode)))
    {
        const ShadowMappingMode modes[] =
        {
            ShadowMappingMode::NONE,
            ShadowMappingMode::DIRECTIONAL_SHADOW,
            ShadowMappingMode::OMNI_SHADOW,
            ShadowMappingMode::CASCADE_SHADOW
        };

        for (const auto mode : modes)
        {
            const bool selected = (_asset.Shadow.Mode == mode);
            if (ImGui::Selectable(GetShadowModeLabel(mode), selected))
            {
                _asset.Shadow.Mode = mode;
                _isDirty = true;
            }

            if (selected)
                ImGui::SetItemDefaultFocus();
        }

        ImGui::EndCombo();
    }
}

bool ShaderEditorPanel::Save()
{
    _asset.FilePath = QEProjectManager::ToProjectRelativePath(_shaderPath);

    if (!QEShaderYamlHelper::WriteShaderFile(_shaderPath, _asset))
    {
        QE_LOG_ERROR_CAT_F("ShaderEditorPanel", "Could not save shader asset {}", _shaderPath.string());
        return false;
    }

    MaterialManager::getInstance()->ReloadShaderAsset(_shaderPath);

    _isDirty = false;
    return true;
}

bool ShaderEditorPanel::DrawStagePathField(const char* label, std::string& value)
{
    bool changed = false;

    std::array<char, 512> buffer{};
    std::strncpy(buffer.data(), value.c_str(), buffer.size() - 1);
    if (ImGui::InputText(label, buffer.data(), buffer.size()))
    {
        value = buffer.data();
        _isDirty = true;
        changed = true;
    }

    ImGui::SameLine();
    const std::string browseButtonId = std::string("...##Browse") + label;
    const std::string popupId = std::string("ShaderAssetPicker##") + label;
    if (ImGui::Button(browseButtonId.c_str()))
    {
        ImGui::OpenPopup(popupId.c_str());
    }

    if (DrawAssetPickerPopup(popupId.c_str(), value))
    {
        _isDirty = true;
        changed = true;
    }

    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("QE_PROJECT_ASSET_PATH"))
        {
            const char* payloadPath = static_cast<const char*>(payload->Data);
            if (payloadPath)
            {
                const std::filesystem::path droppedPath(payloadPath);
                if (IsSpirvPath(droppedPath))
                {
                    value = QEProjectManager::ToProjectRelativePath(droppedPath);
                    _isDirty = true;
                    changed = true;
                }
            }
        }

        ImGui::EndDragDropTarget();
    }

    ImGui::SameLine();
    const std::string clearButtonId = std::string("Clear##") + label;
    if (ImGui::Button(clearButtonId.c_str()))
    {
        value.clear();
        _isDirty = true;
        changed = true;
    }

    return changed;
}

bool ShaderEditorPanel::DrawAssetPickerPopup(const char* popupId, std::string& value)
{
    bool changed = false;

    if (!ImGui::BeginPopup(popupId))
        return false;

    static std::array<char, 256> searchBuffer{};
    ImGui::InputTextWithHint("##ShaderSearch", "Search .spv...", searchBuffer.data(), searchBuffer.size());
    ImGui::Separator();

    const std::string search = [&]()
    {
        std::string v = searchBuffer.data();
        std::transform(v.begin(), v.end(), v.begin(), ::tolower);
        return v;
    }();

    std::vector<std::filesystem::path> candidatePaths;
    const std::filesystem::path projectRoot = QEProjectManager::GetCurrentProjectPath();

    if (!projectRoot.empty() && std::filesystem::exists(projectRoot))
    {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(projectRoot))
        {
            if (!entry.is_regular_file())
                continue;

            if (!IsSpirvPath(entry.path()))
                continue;

            const std::string relativePath = QEProjectManager::ToProjectRelativePath(entry.path());
            std::string lowered = relativePath;
            std::transform(lowered.begin(), lowered.end(), lowered.begin(), ::tolower);

            if (!search.empty() && lowered.find(search) == std::string::npos)
                continue;

            candidatePaths.push_back(entry.path());
        }
    }

    std::sort(candidatePaths.begin(), candidatePaths.end());

    if (ImGui::BeginChild("ShaderAssetPickerList", ImVec2(520.0f, 260.0f), true))
    {
        for (const auto& candidatePath : candidatePaths)
        {
            const std::string relativePath = QEProjectManager::ToProjectRelativePath(candidatePath);
            if (ImGui::Selectable(relativePath.c_str()))
            {
                value = relativePath;
                changed = true;
                std::fill(searchBuffer.begin(), searchBuffer.end(), '\0');
                ImGui::CloseCurrentPopup();
                break;
            }
        }
    }
    ImGui::EndChild();

    if (ImGui::Button("Close"))
    {
        std::fill(searchBuffer.begin(), searchBuffer.end(), '\0');
        ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
    return changed;
}
