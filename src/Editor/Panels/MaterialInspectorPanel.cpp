#include "MaterialInspectorPanel.h"

#include <imgui.h>

#include <GameObjectManager.h>
#include <QEGameObject.h>
#include <Material.h>
#include <MaterialData.h>

#include <Editor/Core/EditorContext.h>
#include <Editor/Core/EditorSelectionManager.h>

namespace
{
    const char* GetAlphaModeLabel(uint32_t alphaMode)
    {
        switch (alphaMode)
        {
        case 1: return "Mask";
        case 2: return "Blend";
        default: return "Opaque";
        }
    }

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
}

MaterialInspectorPanel::MaterialInspectorPanel(
    GameObjectManager* gameObjectManager,
    EditorContext* editorContext,
    EditorSelectionManager* selectionManager)
    : gameObjectManager(gameObjectManager)
    , editorContext(editorContext)
    , selectionManager(selectionManager)
{
}

void MaterialInspectorPanel::Draw()
{
    if (!editorContext || !editorContext->ShowMaterialInspector)
    {
        return;
    }

    ImGui::Begin("Material Inspector", &editorContext->ShowMaterialInspector);

    if (!gameObjectManager)
    {
        ImGui::TextUnformatted("GameObjectManager is null.");
        ImGui::End();
        return;
    }

    if (selectionManager && selectionManager->IsAtmosphereSelected())
    {
        ImGui::TextUnformatted("Atmosphere has no bound materials.");
        ImGui::End();
        return;
    }

    if (!selectionManager || !selectionManager->HasSelection())
    {
        ImGui::TextUnformatted("No GameObject selected.");
        ImGui::End();
        return;
    }

    auto gameObject = selectionManager->GetSelectedGameObject();

    if (!gameObject)
    {
        ImGui::TextUnformatted("Selected GameObject no longer exists.");
        if (selectionManager)
        {
            selectionManager->ClearSelection();
        }
        ImGui::End();
        return;
    }

    const auto& materials = gameObject->GetMaterials();

    ImGui::Text("GameObject: %s", gameObject->Name.c_str());
    ImGui::Text("Materials: %d", static_cast<int>(materials.size()));
    ImGui::Separator();

    if (materials.empty())
    {
        ImGui::TextUnformatted("This GameObject has no bound materials.");
        ImGui::End();
        return;
    }

    for (int i = 0; i < static_cast<int>(materials.size()); ++i)
    {
        DrawMaterial(materials[i], i);
    }

    ImGui::End();
}

void MaterialInspectorPanel::DrawMaterial(const std::shared_ptr<QEMaterial>& material, int materialIndex)
{
    if (!material)
        return;

    const std::string headerLabel = material->Name + "##Material_" + std::to_string(materialIndex);

    if (!ImGui::CollapsingHeader(headerLabel.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Separator();
        return;
    }

    ImGui::Text("Name: %s", material->Name.c_str());
    ImGui::Text("Shader: %s", material->shader ? material->shader->shaderNameID.c_str() : "None");

    int currentQueueIndex = 0;
    for (int i = 0; i < IM_ARRAYSIZE(kRenderQueueValues); ++i)
    {
        if (material->renderQueue == kRenderQueueValues[i])
        {
            currentQueueIndex = i;
            break;
        }
    }

    if (ImGui::BeginCombo(
        ("Render Queue##RenderQueue_" + std::to_string(materialIndex)).c_str(),
        GetRenderQueueLabel(material->renderQueue)))
    {
        for (int i = 0; i < IM_ARRAYSIZE(kRenderQueueValues); ++i)
        {
            const bool isSelected = (material->renderQueue == kRenderQueueValues[i]);

            if (ImGui::Selectable(kRenderQueueLabels[i], isSelected))
            {
                material->renderQueue = kRenderQueueValues[i];
            }

            if (isSelected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }

        ImGui::EndCombo();
    }

    int renderQueueValue = static_cast<int>(material->renderQueue);
    if (ImGui::DragInt(
        ("Render Queue Value##RenderQueueValue_" + std::to_string(materialIndex)).c_str(),
        &renderQueueValue,
        10.0f,
        0,
        100000))
    {
        renderQueueValue = std::max(0, renderQueueValue);
        material->renderQueue = static_cast<unsigned int>(renderQueueValue);
    }

    ImGui::Separator();
    ImGui::TextUnformatted("Colors");

    glm::vec3 diffuse(material->materialData.Diffuse);
    if (ImGui::ColorEdit3(("Diffuse##Diffuse_" + std::to_string(materialIndex)).c_str(), &diffuse.x))
    {
        material->materialData.SetMaterialField("Diffuse", diffuse);
    }

    glm::vec3 ambient(material->materialData.Ambient);
    if (ImGui::ColorEdit3(("Ambient##Ambient_" + std::to_string(materialIndex)).c_str(), &ambient.x))
    {
        material->materialData.SetMaterialField("Ambient", ambient);
    }

    glm::vec3 specular(material->materialData.Specular);
    if (ImGui::ColorEdit3(("Specular##Specular_" + std::to_string(materialIndex)).c_str(), &specular.x))
    {
        material->materialData.SetMaterialField("Specular", specular);
    }

    glm::vec3 emissive(material->materialData.Emissive);
    if (ImGui::ColorEdit3(("Emissive##Emissive_" + std::to_string(materialIndex)).c_str(), &emissive.x))
    {
        material->materialData.SetMaterialField("Emissive", emissive);
    }

    ImGui::Separator();
    ImGui::TextUnformatted("Surface");

    float opacity = material->materialData.Opacity;
    if (ImGui::DragFloat(("Opacity##Opacity_" + std::to_string(materialIndex)).c_str(), &opacity, 0.01f, 0.0f, 1.0f))
    {
        material->materialData.SetMaterialField("Opacity", opacity);
    }

    float metallic = material->materialData.Metallic;
    if (ImGui::DragFloat(("Metallic##Metallic_" + std::to_string(materialIndex)).c_str(), &metallic, 0.01f, 0.0f, 1.0f))
    {
        material->materialData.SetMaterialField("Metallic", metallic);
    }

    float roughness = material->materialData.Roughness;
    if (ImGui::DragFloat(("Roughness##Roughness_" + std::to_string(materialIndex)).c_str(), &roughness, 0.01f, 0.0f, 1.0f))
    {
        material->materialData.SetMaterialField("Roughness", roughness);
    }

    float ao = material->materialData.AO;
    if (ImGui::DragFloat(("AO##AO_" + std::to_string(materialIndex)).c_str(), &ao, 0.01f, 0.0f, 1.0f))
    {
        material->materialData.SetMaterialField("AO", ao);
    }

    float clearcoat = material->materialData.Clearcoat;
    if (ImGui::DragFloat(("Clearcoat##Clearcoat_" + std::to_string(materialIndex)).c_str(), &clearcoat, 0.01f, 0.0f, 1.0f))
    {
        material->materialData.SetMaterialField("Clearcoat", clearcoat);
    }

    float clearcoatRoughness = material->materialData.ClearcoatRoughness;
    if (ImGui::DragFloat(("ClearcoatRoughness##ClearcoatRoughness_" + std::to_string(materialIndex)).c_str(), &clearcoatRoughness, 0.01f, 0.0f, 1.0f))
    {
        material->materialData.SetMaterialField("ClearcoatRoughness", clearcoatRoughness);
    }

    float alphaCutoff = material->materialData.AlphaCutoff;
    if (ImGui::DragFloat(("AlphaCutoff##AlphaCutoff_" + std::to_string(materialIndex)).c_str(), &alphaCutoff, 0.01f, 0.0f, 1.0f))
    {
        material->materialData.SetMaterialField("AlphaCutoff", alphaCutoff);
    }

    if (ImGui::BeginCombo(
        ("Alpha Mode##AlphaMode_" + std::to_string(materialIndex)).c_str(),
        GetAlphaModeLabel(material->materialData.AlphaMode)))
    {
        const char* labels[] = { "Opaque", "Mask", "Blend" };
        for (int i = 0; i < 3; ++i)
        {
            const bool isSelected = (static_cast<int>(material->materialData.AlphaMode) == i);
            if (ImGui::Selectable(labels[i], isSelected))
            {
                material->materialData.SetMaterialField("AlphaMode", i);
                if (i == 2 && material->renderQueue < static_cast<unsigned int>(RenderQueue::Transparent))
                {
                    material->renderQueue = static_cast<unsigned int>(RenderQueue::Transparent);
                }
                else if (i != 2 && material->renderQueue == static_cast<unsigned int>(RenderQueue::Transparent))
                {
                    material->renderQueue = static_cast<unsigned int>(RenderQueue::Geometry);
                }
            }

            if (isSelected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }

        ImGui::EndCombo();
    }

    bool doubleSided = material->materialData.DoubleSided;
    if (ImGui::Checkbox(("Double Sided##DoubleSided_" + std::to_string(materialIndex)).c_str(), &doubleSided))
    {
        material->materialData.SetMaterialField("DoubleSided", doubleSided ? 1 : 0);
    }

    ImGui::Separator();
}
