#include "MaterialEditorPanel.h"

#include <algorithm>

#include <imgui.h>

#include <DeviceModule.h>
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
    }
    else
    {
        ImGui::Dummy(previewSize);
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

    if (changed)
    {
        SyncPreview();
    }
}

void MaterialEditorPanel::SyncPreview()
{
    _previewRenderer.SyncFromMaterial(_material);
}
