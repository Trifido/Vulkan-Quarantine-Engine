#include "MaterialInspectorPanel.h"

#include <imgui.h>

#include <GameObjectManager.h>
#include <QEGameObject.h>
#include <Material.h>

#include <Editor/Core/EditorContext.h>
#include <Editor/Core/EditorSelectionManager.h>

MaterialInspectorPanel::MaterialInspectorPanel(
    GameObjectManager* gameObjectManager,
    EditorContext* editorContext,
    EditorSelectionManager* selectionManager,
    std::function<void(const std::shared_ptr<QEMaterial>&)> onOpenMaterialEditor)
    : gameObjectManager(gameObjectManager)
    , editorContext(editorContext)
    , selectionManager(selectionManager)
    , onOpenMaterialEditor(std::move(onOpenMaterialEditor))
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
    ImGui::TextUnformatted("Double click a material to open the Material Editor.");
    ImGui::Spacing();

    if (materials.empty())
    {
        ImGui::TextUnformatted("This GameObject has no bound materials.");
        ImGui::End();
        return;
    }

    for (int i = 0; i < static_cast<int>(materials.size()); ++i)
    {
        DrawMaterialEntry(materials[i], i);
    }

    ImGui::End();
}

void MaterialInspectorPanel::DrawMaterialEntry(const std::shared_ptr<QEMaterial>& material, int materialIndex)
{
    if (!material)
        return;

    ImGui::PushID(materialIndex);

    const std::string shaderName = material->shader ? material->shader->shaderNameID : "None";
    const std::string label = material->Name + " [" + shaderName + "]";

    ImGui::Selectable(label.c_str(), false);

    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
    {
        if (onOpenMaterialEditor)
        {
            onOpenMaterialEditor(material);
        }
    }

    ImGui::PopID();
}
