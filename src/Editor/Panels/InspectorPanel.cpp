#include "InspectorPanel.h"

#include <imgui.h>
#include <GameObjectManager.h>
#include <QEGameObject.h>
#include <QETransform.h>
#include "Editor/Core/EditorContext.h"

InspectorPanel::InspectorPanel(GameObjectManager* gameObjectManager, EditorContext* editorContext)
    : gameObjectManager(gameObjectManager), editorContext(editorContext)
{
}

void InspectorPanel::Draw()
{
    if (!editorContext || !editorContext->ShowInspector)
    {
        return;
    }

    ImGui::Begin("Inspector", &editorContext->ShowInspector);

    if (!gameObjectManager)
    {
        ImGui::TextUnformatted("GameObjectManager is null.");
        ImGui::End();
        return;
    }

    if (editorContext->SelectedGameObjectId.empty())
    {
        ImGui::TextUnformatted("No GameObject selected.");
        ImGui::End();
        return;
    }

    auto gameObject = gameObjectManager->GetGameObjectById(editorContext->SelectedGameObjectId);

    if (!gameObject)
    {
        ImGui::TextUnformatted("Selected GameObject no longer exists.");
        editorContext->SelectedGameObjectId.clear();
        ImGui::End();
        return;
    }

    ImGui::Text("Name: %s", gameObject->Name.c_str());
    ImGui::Text("ID: %s", gameObject->ID().c_str());
    ImGui::Text("Children: %d", static_cast<int>(gameObject->childs.size()));
    ImGui::Text("Components: %d", static_cast<int>(gameObject->components.size()));

    ImGui::Separator();

    if (auto transform = gameObject->GetComponent<QETransform>())
    {
        glm::vec3 position = transform->localPosition;
        glm::vec3 rotation = glm::degrees(glm::eulerAngles(transform->localRotation));
        glm::vec3 scale = transform->localScale;

        ImGui::TextUnformatted("Transform");

        ImGui::InputFloat3("Position", &position.x, "%.3f", ImGuiInputTextFlags_ReadOnly);
        ImGui::InputFloat3("Rotation", &rotation.x, "%.3f", ImGuiInputTextFlags_ReadOnly);
        ImGui::InputFloat3("Scale", &scale.x, "%.3f", ImGuiInputTextFlags_ReadOnly);
    }
    else
    {
        ImGui::TextUnformatted("No Transform component.");
    }

    ImGui::End();
}
