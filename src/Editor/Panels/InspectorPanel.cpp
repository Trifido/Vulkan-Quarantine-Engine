#include "InspectorPanel.h"

#include <imgui.h>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/quaternion.hpp>

#include <GameObjectManager.h>
#include <QEGameObject.h>
#include <QETransform.h>
#include <Editor/Core/EditorContext.h>
#include <Editor/Core/EditorSelectionManager.h>

InspectorPanel::InspectorPanel(
    GameObjectManager* gameObjectManager,
    EditorContext* editorContext,
    EditorSelectionManager* selectionManager)
    : gameObjectManager(gameObjectManager)
    , editorContext(editorContext)
    , selectionManager(selectionManager)
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
        selectionManager->ClearSelection();
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

        if (ImGui::DragFloat3("Position", &position.x, 0.1f))
        {
            transform->SetLocalPosition(position);
        }

        if (ImGui::DragFloat3("Rotation", &rotation.x, 0.5f))
        {
            transform->SetLocalEulerDegrees(rotation);
        }

        if (ImGui::DragFloat3("Scale", &scale.x, 0.05f))
        {
            scale.x = (scale.x == 0.0f) ? 0.0001f : scale.x;
            scale.y = (scale.y == 0.0f) ? 0.0001f : scale.y;
            scale.z = (scale.z == 0.0f) ? 0.0001f : scale.z;

            transform->SetLocalScale(scale);
        }
    }
    else
    {
        ImGui::TextUnformatted("No Transform component.");
    }

    ImGui::End();
}
