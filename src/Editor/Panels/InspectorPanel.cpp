#include "InspectorPanel.h"

#include <imgui.h>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/quaternion.hpp>

#include <GameObjectManager.h>
#include <QEGameObject.h>
#include <QETransform.h>
#include <Editor/Core/EditorContext.h>
#include <Editor/Core/EditorSelectionManager.h>

#include <Editor/Commands/EditorCommandManager.h>
#include <Editor/Commands/TransformCommand.h>
#include <Editor/Commands/EditorTransformUtils.h>

InspectorPanel::InspectorPanel(
    GameObjectManager* gameObjectManager,
    EditorContext* editorContext,
    EditorSelectionManager* selectionManager,
    EditorCommandManager* commandManager)
    : gameObjectManager(gameObjectManager)
    , editorContext(editorContext)
    , selectionManager(selectionManager)
    , commandManager(commandManager)
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

    auto transform = gameObject->GetComponent<QETransform>();
    if (!transform)
    {
        ImGui::TextUnformatted("No Transform component.");
        ImGui::End();
        return;
    }

    glm::vec3 position = transform->localPosition;
    glm::vec3 rotation = glm::degrees(glm::eulerAngles(transform->localRotation));
    glm::vec3 scale = transform->localScale;

    ImGui::TextUnformatted("Transform");

    if (ImGui::DragFloat3("Position", &position.x, 0.1f))
    {
        if (commandManager)
        {
            TransformState before = EditorTransformUtils::CaptureState(transform);
            TransformState after = before;
            after.Position = position;

            commandManager->ExecuteCommand(
                std::make_unique<TransformCommand>(
                    gameObject->ID(),
                    before,
                    after));
        }
        else
        {
            transform->SetLocalPosition(position);
        }
    }

    if (ImGui::DragFloat3("Rotation", &rotation.x, 0.5f))
    {
        if (commandManager)
        {
            TransformState before = EditorTransformUtils::CaptureState(transform);
            TransformState after = before;
            after.Rotation = glm::quat(glm::radians(rotation));

            commandManager->ExecuteCommand(
                std::make_unique<TransformCommand>(
                    gameObject->ID(),
                    before,
                    after));
        }
        else
        {
            transform->SetLocalEulerDegrees(rotation);
        }
    }

    if (ImGui::DragFloat3("Scale", &scale.x, 0.05f))
    {
        scale = glm::max(scale, glm::vec3(0.0001f));

        if (commandManager)
        {
            TransformState before = EditorTransformUtils::CaptureState(transform);
            TransformState after = before;
            after.Scale = scale;

            commandManager->ExecuteCommand(
                std::make_unique<TransformCommand>(
                    gameObject->ID(),
                    before,
                    after));
        }
        else
        {
            transform->SetLocalScale(scale);
        }
    }

    ImGui::End();
}
