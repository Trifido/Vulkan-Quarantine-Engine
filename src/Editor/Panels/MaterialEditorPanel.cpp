#include "MaterialEditorPanel.h"

#include <algorithm>
#include <array>
#include <vector>

#include <imgui.h>

#include <DeviceModule.h>
#include <QEProjectManager.h>
#include <QEShaderAssetLoader.h>
#include <RenderPassModule.h>
#include <CommandPoolModule.h>
#include <QueueModule.h>
#include <MaterialManager.h>
#include <Material.h>
#include <MaterialData.h>

#include <Editor/Core/EditorContext.h>

namespace
{
    const char* GetPreviewShapeLabel(MaterialPreviewRenderer::PreviewShape shape)
    {
        switch (shape)
        {
        case MaterialPreviewRenderer::PreviewShape::Plane: return "Plane";
        case MaterialPreviewRenderer::PreviewShape::Cube:  return "Cube";
        case MaterialPreviewRenderer::PreviewShape::Sphere:
        default: return "Sphere";
        }
    }

    const char* GetAlphaModeLabel(uint32_t alphaMode)
    {
        switch (alphaMode)
        {
        case 1: return "Mask";
        case 2: return "Blend";
        default: return "Opaque";
        }
    }

    const unsigned int kRenderQueueValues[] =
    {
        1000, 2000, 3000, 3100, 4000, 5000, 6000
    };

    const char* kRenderQueueLabels[] =
    {
        "Background",
        "Geometry",
        "Transparent",
        "Particles",
        "UI",
        "Debug",
        "Editor"
    };

    const char* GetRenderQueueLabel(unsigned int renderQueue)
    {
        switch (renderQueue)
        {
        case 1000: return "Background";
        case 2000: return "Geometry";
        case 3000: return "Transparent";
        case 3100: return "Particles";
        case 4000: return "UI";
        case 5000: return "Debug";
        case 6000: return "Editor";
        default:   return "Custom";
        }
    }
}

MaterialEditorPanel::MaterialEditorPanel(
    EditorContext* editorContext,
    MaterialManager* materialManager,
    DeviceModule* deviceModule,
    RenderPassModule* renderPassModule,
    CommandPoolModule* commandPoolModule,
    QueueModule* queueModule)
    : editorContext(editorContext)
    , materialManager(materialManager)
{
    _previewRenderer.Initialize(deviceModule, renderPassModule, commandPoolModule, queueModule);
}

MaterialEditorPanel::~MaterialEditorPanel()
{
    _previewRenderer.Cleanup();
}

void MaterialEditorPanel::OpenMaterial(const std::shared_ptr<QEMaterial>& material)
{
    _material = material;

    if (editorContext)
    {
        editorContext->ShowMaterialEditor = (_material != nullptr);
    }

    _previewRenderer.SetMaterial(_material);
    SyncPreview();
}

bool MaterialEditorPanel::OpenMaterialFromFile(const std::filesystem::path& materialPath)
{
    if (!materialManager)
        return false;

    auto material = materialManager->LoadMaterialFromFile(materialPath);
    if (!material)
        return false;

    _materialPath = materialPath;
    OpenMaterial(material);
    return true;
}

void MaterialEditorPanel::RebuildPreview()
{
    _previewRenderer.Rebuild();
}

void MaterialEditorPanel::RenderPreview(VkCommandBuffer& commandBuffer, uint32_t currentFrame)
{
    if (!editorContext || !editorContext->ShowMaterialEditor)
        return;

    _previewRenderer.Render(commandBuffer, currentFrame);
}

void MaterialEditorPanel::Draw()
{
    if (!editorContext || !editorContext->ShowMaterialEditor)
    {
        return;
    }

    if (!ImGui::Begin("Material Editor", &editorContext->ShowMaterialEditor))
    {
        ImGui::End();
        return;
    }

    if (!_material)
    {
        ImGui::TextUnformatted("No material selected.");
        ImGui::End();
        return;
    }

    DrawToolbar();
    ImGui::Separator();

    if (ImGui::BeginTable("MaterialEditorLayout", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchProp))
    {
        ImGui::TableSetupColumn("Preview", ImGuiTableColumnFlags_WidthStretch, 0.62f);
        ImGui::TableSetupColumn("Properties", ImGuiTableColumnFlags_WidthStretch, 0.38f);

        ImGui::TableNextColumn();
        DrawPreview();

        ImGui::TableNextColumn();
        DrawProperties();

        ImGui::EndTable();
    }

    ImGui::End();
}

void MaterialEditorPanel::DrawToolbar()
{
    ImGui::Text("Material: %s", _material->Name.c_str());
    ImGui::Text("Shader: %s", _material->shader ? _material->shader->shaderNameID.c_str() : "None");

    const std::string shaderAssetLabel = _material->GetShaderAssetPath().empty()
        ? "Legacy runtime shader"
        : _material->GetShaderAssetPath();
    ImGui::TextWrapped("Shader Asset: %s", shaderAssetLabel.c_str());

    if (ImGui::Button("Select Shader"))
    {
        ImGui::OpenPopup("MaterialShaderPicker");
    }

    DrawShaderPickerPopup("MaterialShaderPicker");

    if (ImGui::Button("Save"))
    {
        _material->SaveMaterialFile();
    }

    ImGui::SameLine();
    ImGui::TextUnformatted("Preview");

    ImGui::SameLine();

    if (ImGui::BeginCombo("##PreviewShape", GetPreviewShapeLabel(_previewRenderer.GetPreviewShape())))
    {
        const MaterialPreviewRenderer::PreviewShape shapes[] =
        {
            MaterialPreviewRenderer::PreviewShape::Sphere,
            MaterialPreviewRenderer::PreviewShape::Plane,
            MaterialPreviewRenderer::PreviewShape::Cube
        };

        for (const auto shape : shapes)
        {
            const bool selected = (_previewRenderer.GetPreviewShape() == shape);
            if (ImGui::Selectable(GetPreviewShapeLabel(shape), selected))
            {
                _previewRenderer.SetPreviewShape(shape);
            }

            if (selected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }

        ImGui::EndCombo();
    }
}

void MaterialEditorPanel::DrawPreview()
{
    ImVec2 avail = ImGui::GetContentRegionAvail();
    ImVec2 previewSize(std::max(1.0f, avail.x), std::max(260.0f, avail.y));

    _previewRenderer.Resize(
        std::max(1u, static_cast<uint32_t>(previewSize.x)),
        std::max(1u, static_cast<uint32_t>(previewSize.y)));

    if (_previewRenderer.IsReady())
    {
        ImGui::Image(_previewRenderer.GetImGuiTexture(), previewSize);
        HandlePreviewInteraction();
    }
    else
    {
        ImGui::Dummy(previewSize);
    }
}

void MaterialEditorPanel::HandlePreviewInteraction()
{
    if (!ImGui::IsItemHovered())
        return;

    ImGuiIO& io = ImGui::GetIO();

    if (ImGui::IsMouseDragging(ImGuiMouseButton_Left))
    {
        _previewRenderer.Orbit(-io.MouseDelta.x * 0.35f, -io.MouseDelta.y * 0.35f);
    }

    if (io.MouseWheel != 0.0f)
    {
        _previewRenderer.Zoom(-io.MouseWheel * 0.2f);
    }
}

void MaterialEditorPanel::DrawProperties()
{
    bool changed = false;

    if (ImGui::BeginCombo("Render Queue", GetRenderQueueLabel(_material->renderQueue)))
    {
        for (int i = 0; i < IM_ARRAYSIZE(kRenderQueueValues); ++i)
        {
            const bool isSelected = (_material->renderQueue == kRenderQueueValues[i]);

            if (ImGui::Selectable(kRenderQueueLabels[i], isSelected))
            {
                _material->renderQueue = kRenderQueueValues[i];
                changed = true;
            }

            if (isSelected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }

        ImGui::EndCombo();
    }

    int renderQueueValue = static_cast<int>(_material->renderQueue);
    if (ImGui::DragInt("Render Queue Value", &renderQueueValue, 10.0f, 0, 100000))
    {
        renderQueueValue = std::max(0, renderQueueValue);
        _material->renderQueue = static_cast<unsigned int>(renderQueueValue);
        changed = true;
    }

    ImGui::SeparatorText("Colors");

    glm::vec3 diffuse(_material->materialData.Diffuse);
    if (ImGui::ColorEdit3("Diffuse", &diffuse.x))
    {
        _material->materialData.SetMaterialField("Diffuse", diffuse);
        changed = true;
    }

    glm::vec3 ambient(_material->materialData.Ambient);
    if (ImGui::ColorEdit3("Ambient", &ambient.x))
    {
        _material->materialData.SetMaterialField("Ambient", ambient);
        changed = true;
    }

    glm::vec3 specular(_material->materialData.Specular);
    if (ImGui::ColorEdit3("Specular", &specular.x))
    {
        _material->materialData.SetMaterialField("Specular", specular);
        changed = true;
    }

    glm::vec3 emissive(_material->materialData.Emissive);
    if (ImGui::ColorEdit3("Emissive", &emissive.x))
    {
        _material->materialData.SetMaterialField("Emissive", emissive);
        changed = true;
    }

    ImGui::SeparatorText("Surface");

    float opacity = _material->materialData.Opacity;
    if (ImGui::DragFloat("Opacity", &opacity, 0.01f, 0.0f, 1.0f))
    {
        _material->materialData.SetMaterialField("Opacity", opacity);
        changed = true;
    }

    float metallic = _material->materialData.Metallic;
    if (ImGui::DragFloat("Metallic", &metallic, 0.01f, 0.0f, 1.0f))
    {
        _material->materialData.SetMaterialField("Metallic", metallic);
        changed = true;
    }

    float roughness = _material->materialData.Roughness;
    if (ImGui::DragFloat("Roughness", &roughness, 0.01f, 0.0f, 1.0f))
    {
        _material->materialData.SetMaterialField("Roughness", roughness);
        changed = true;
    }

    float ao = _material->materialData.AO;
    if (ImGui::DragFloat("AO", &ao, 0.01f, 0.0f, 1.0f))
    {
        _material->materialData.SetMaterialField("AO", ao);
        changed = true;
    }

    float clearcoat = _material->materialData.Clearcoat;
    if (ImGui::DragFloat("Clearcoat", &clearcoat, 0.01f, 0.0f, 1.0f))
    {
        _material->materialData.SetMaterialField("Clearcoat", clearcoat);
        changed = true;
    }

    float clearcoatRoughness = _material->materialData.ClearcoatRoughness;
    if (ImGui::DragFloat("Clearcoat Roughness", &clearcoatRoughness, 0.01f, 0.0f, 1.0f))
    {
        _material->materialData.SetMaterialField("ClearcoatRoughness", clearcoatRoughness);
        changed = true;
    }

    float alphaCutoff = _material->materialData.AlphaCutoff;
    if (ImGui::DragFloat("Alpha Cutoff", &alphaCutoff, 0.01f, 0.0f, 1.0f))
    {
        _material->materialData.SetMaterialField("AlphaCutoff", alphaCutoff);
        changed = true;
    }

    if (ImGui::BeginCombo("Alpha Mode", GetAlphaModeLabel(_material->materialData.AlphaMode)))
    {
        const char* labels[] = { "Opaque", "Mask", "Blend" };
        for (int i = 0; i < 3; ++i)
        {
            const bool isSelected = (static_cast<int>(_material->materialData.AlphaMode) == i);
            if (ImGui::Selectable(labels[i], isSelected))
            {
                _material->materialData.SetMaterialField("AlphaMode", i);

                if (i == 2 && _material->renderQueue < static_cast<unsigned int>(RenderQueue::Transparent))
                {
                    _material->renderQueue = static_cast<unsigned int>(RenderQueue::Transparent);
                }
                else if (i != 2 && _material->renderQueue == static_cast<unsigned int>(RenderQueue::Transparent))
                {
                    _material->renderQueue = static_cast<unsigned int>(RenderQueue::Geometry);
                }

                changed = true;
            }

            if (isSelected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }

        ImGui::EndCombo();
    }

    bool doubleSided = _material->materialData.DoubleSided;
    if (ImGui::Checkbox("Double Sided", &doubleSided))
    {
        _material->materialData.SetMaterialField("DoubleSided", doubleSided ? 1 : 0);
        changed = true;
    }

    DrawShaderReflection();

    if (changed)
    {
        SyncPreview();
    }
}

void MaterialEditorPanel::DrawShaderReflection()
{
    if (!_material || !_material->shader)
        return;

    auto& reflect = _material->shader->reflectShader;

    ImGui::SeparatorText("Shader Reflection");

    ImGui::Text("Material UBO: %s", reflect.isUBOMaterial ? "Yes" : "No");
    ImGui::Text("Input Stride: %u", reflect.inputStrideSize);
    ImGui::Text("Directional Shadows: %s", reflect.HasDirectionalShadows ? "Yes" : "No");
    ImGui::Text("Point Shadows: %s", reflect.HasPointShadows ? "Yes" : "No");
    ImGui::Text("Spot Shadows: %s", reflect.HasSpotShadows ? "Yes" : "No");

    if (!reflect.isUBOMaterial)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 210, 120, 255));
        ImGui::TextWrapped("This shader does not expose the expected material UBO. Material properties may not affect rendering.");
        ImGui::PopStyleColor();
    }

    if (ImGui::TreeNode("Descriptor Bindings"))
    {
        for (const auto& [setIndex, bindingMap] : reflect.bindings)
        {
            const std::string setLabel = "Set " + std::to_string(setIndex);
            if (ImGui::TreeNode(setLabel.c_str()))
            {
                for (const auto& [bindingIndex, binding] : bindingMap)
                {
                    ImGui::BulletText(
                        "Binding %u - %s (array: %u)",
                        bindingIndex,
                        binding.name.c_str(),
                        binding.arraySize);
                }

                ImGui::TreePop();
            }
        }

        ImGui::TreePop();
    }

    if (!reflect.materialUBOMembers.empty() && ImGui::TreeNode("Material UBO Members"))
    {
        for (const auto& member : reflect.materialUBOMembers)
        {
            ImGui::BulletText(
                "%s (offset: %u, size: %u)",
                member.name.c_str(),
                member.offset,
                member.size);
        }

        ImGui::TreePop();
    }

    if (!reflect.inputVariables.empty() && ImGui::TreeNode("Vertex Inputs"))
    {
        for (const auto& input : reflect.inputVariables)
        {
            ImGui::BulletText(
                "Location %u - %s [%s] (%u bytes)",
                input.location,
                input.name.c_str(),
                input.type.c_str(),
                input.size);
        }

        ImGui::TreePop();
    }
}

bool MaterialEditorPanel::DrawShaderPickerPopup(const char* popupId)
{
    if (!_material)
        return false;

    if (!ImGui::BeginPopup(popupId))
        return false;

    static std::array<char, 256> searchBuffer{};
    ImGui::InputTextWithHint("##MaterialShaderSearch", "Search .qeshader...", searchBuffer.data(), searchBuffer.size());
    ImGui::Separator();

    std::string search = searchBuffer.data();
    std::transform(search.begin(), search.end(), search.begin(), ::tolower);

    std::vector<std::filesystem::path> candidatePaths;
    const std::filesystem::path projectRoot = QEProjectManager::GetCurrentProjectPath();

    if (!projectRoot.empty() && std::filesystem::exists(projectRoot))
    {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(projectRoot))
        {
            if (!entry.is_regular_file())
                continue;

            if (!QEShaderAssetLoader::IsShaderAssetPath(entry.path()))
                continue;

            std::string relativePath = QEProjectManager::ToProjectRelativePath(entry.path());
            std::string lowered = relativePath;
            std::transform(lowered.begin(), lowered.end(), lowered.begin(), ::tolower);

            if (!search.empty() && lowered.find(search) == std::string::npos)
                continue;

            candidatePaths.push_back(entry.path());
        }
    }

    std::sort(candidatePaths.begin(), candidatePaths.end());

    bool changed = false;

    if (ImGui::BeginChild("MaterialShaderPickerList", ImVec2(520.0f, 260.0f), true))
    {
        for (const auto& candidatePath : candidatePaths)
        {
            const std::string relativePath = QEProjectManager::ToProjectRelativePath(candidatePath);
            if (ImGui::Selectable(relativePath.c_str()))
            {
                auto shader = QEShaderAssetLoader::LoadShaderAsset(relativePath);
                if (shader && _material->ApplyShader(shader, relativePath))
                {
                    changed = true;
                    SyncPreview();
                }

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

void MaterialEditorPanel::SyncPreview()
{
    _previewRenderer.SyncFromMaterial(_material);
}
