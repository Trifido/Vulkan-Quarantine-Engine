#include "MaterialInspectorPanel.h"

#include <algorithm>
#include <array>
#include <vector>

#include <imgui.h>

#include <GameObjectManager.h>
#include <QEGameObject.h>
#include <Material.h>
#include <MaterialManager.h>
#include <QEProjectManager.h>

#include <Editor/Core/EditorContext.h>
#include <Editor/Core/EditorSelectionManager.h>

MaterialInspectorPanel::MaterialInspectorPanel(
    GameObjectManager* gameObjectManager,
    MaterialManager* materialManager,
    EditorContext* editorContext,
    EditorSelectionManager* selectionManager,
    std::function<void(const std::shared_ptr<QEMaterial>&)> onOpenMaterialEditor)
    : gameObjectManager(gameObjectManager)
    , materialManager(materialManager)
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
        selectedMaterialIndex = -1;
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
        selectedMaterialIndex = -1;
        ImGui::End();
        return;
    }

    const auto& materials = gameObject->GetMaterials();
    if (selectedMaterialIndex >= static_cast<int>(materials.size()))
    {
        selectedMaterialIndex = materials.empty() ? -1 : static_cast<int>(materials.size()) - 1;
    }

    ImGui::Text("GameObject: %s", gameObject->Name.c_str());
    ImGui::Text("Materials: %d", static_cast<int>(materials.size()));
    ImGui::Separator();

    DrawToolbar(gameObject);

    ImGui::Spacing();
    ImGui::TextUnformatted("Double click a material to open the Material Editor.");
    ImGui::Separator();

    if (materials.empty())
    {
        ImGui::TextUnformatted("This GameObject has no bound materials.");
        DrawAddMaterialPopup(gameObject, "AddMaterialPopup");
        ImGui::End();
        return;
    }

    for (int i = 0; i < static_cast<int>(materials.size()); ++i)
    {
        DrawMaterialEntry(materials[i], i);
    }

    DrawAddMaterialPopup(gameObject, "AddMaterialPopup");
    ImGui::End();
}

void MaterialInspectorPanel::DrawToolbar(const std::shared_ptr<QEGameObject>& gameObject)
{
    if (ImGui::Button("Add Material"))
    {
        ImGui::OpenPopup("AddMaterialPopup");
    }

    ImGui::SameLine();

    const bool hasSelection =
        gameObject &&
        selectedMaterialIndex >= 0 &&
        selectedMaterialIndex < static_cast<int>(gameObject->GetMaterials().size());

    ImGui::BeginDisabled(!hasSelection);
    if (ImGui::Button("Delete Material"))
    {
        DeleteSelectedMaterial(gameObject);
    }
    ImGui::EndDisabled();

    if (hasSelection)
    {
        ImGui::SameLine();
        ImGui::Text("Selected: %s", gameObject->GetMaterials()[selectedMaterialIndex]->Name.c_str());
    }

    const bool canDeleteWithKeyboard =
        hasSelection &&
        ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) &&
        ImGui::IsKeyPressed(ImGuiKey_Delete, false) &&
        !ImGui::GetIO().WantTextInput;

    if (canDeleteWithKeyboard)
    {
        DeleteSelectedMaterial(gameObject);
    }
}

void MaterialInspectorPanel::DrawMaterialEntry(const std::shared_ptr<QEMaterial>& material, int materialIndex)
{
    if (!material)
        return;

    ImGui::PushID(materialIndex);

    const std::string shaderName = material->shader ? material->shader->shaderNameID : "None";
    const std::string label = material->Name + " [" + shaderName + "]";

    const bool isSelected = (selectedMaterialIndex == materialIndex);
    if (ImGui::Selectable(label.c_str(), isSelected))
    {
        selectedMaterialIndex = materialIndex;
    }

    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
    {
        if (onOpenMaterialEditor)
        {
            onOpenMaterialEditor(material);
        }
    }

    ImGui::PopID();
}

bool MaterialInspectorPanel::DrawAddMaterialPopup(const std::shared_ptr<QEGameObject>& gameObject, const char* popupId)
{
    if (!gameObject || !materialManager)
        return false;

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    const ImVec2 center = viewport->GetCenter();

    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(520.0f, 360.0f), ImGuiCond_Appearing);

    if (!ImGui::BeginPopupModal(popupId, nullptr, ImGuiWindowFlags_NoResize))
        return false;

    static std::array<char, 256> searchBuffer{};
    static bool setFocusToSearch = true;

    ImGui::TextUnformatted("Add Material");
    ImGui::Separator();

    if (setFocusToSearch)
    {
        ImGui::SetKeyboardFocusHere();
        setFocusToSearch = false;
    }

    ImGui::InputTextWithHint("##GameObjectMaterialSearch", "Search .qemat...", searchBuffer.data(), searchBuffer.size());
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

            std::string extension = entry.path().extension().string();
            std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
            if (extension != ".qemat")
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

    if (ImGui::BeginChild("AddMaterialList", ImVec2(0.0f, -40.0f), true))
    {
        if (candidatePaths.empty())
        {
            ImGui::TextUnformatted("No materials found.");
        }
        else
        {
            for (const auto& candidatePath : candidatePaths)
            {
                const std::string relativePath = QEProjectManager::ToProjectRelativePath(candidatePath);
                if (ImGui::Selectable(relativePath.c_str()))
                {
                    auto material = materialManager->LoadMaterialFromFile(relativePath);
                    if (material && gameObject->AddMaterialBinding(material))
                    {
                        selectedMaterialIndex = static_cast<int>(gameObject->GetMaterials().size()) - 1;
                        changed = true;
                    }

                    std::fill(searchBuffer.begin(), searchBuffer.end(), '\0');
                    setFocusToSearch = true;
                    ImGui::CloseCurrentPopup();
                    break;
                }
            }
        }
    }
    ImGui::EndChild();

    if (ImGui::Button("Close"))
    {
        std::fill(searchBuffer.begin(), searchBuffer.end(), '\0');
        setFocusToSearch = true;
        ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
    return changed;
}

void MaterialInspectorPanel::DeleteSelectedMaterial(const std::shared_ptr<QEGameObject>& gameObject)
{
    if (!gameObject)
        return;

    if (selectedMaterialIndex < 0)
        return;

    if (!gameObject->RemoveMaterialAt(static_cast<size_t>(selectedMaterialIndex)))
        return;

    const auto& materials = gameObject->GetMaterials();
    if (materials.empty())
    {
        selectedMaterialIndex = -1;
    }
    else if (selectedMaterialIndex >= static_cast<int>(materials.size()))
    {
        selectedMaterialIndex = static_cast<int>(materials.size()) - 1;
    }
}
