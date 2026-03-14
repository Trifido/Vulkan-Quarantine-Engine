#include "SceneHierarchyPanel.h"

#include <imgui.h>
#include <GameObjectManager.h>
#include <QEGameObject.h>
#include "EditorContext.h"

SceneHierarchyPanel::SceneHierarchyPanel(GameObjectManager* gameObjectManager, EditorContext* editorContext)
    : gameObjectManager(gameObjectManager), editorContext(editorContext)
{
}

void SceneHierarchyPanel::Draw()
{
    if (!editorContext || !editorContext->ShowHierarchy)
    {
        return;
    }

    ImGui::Begin("Scene Hierarchy", &editorContext->ShowHierarchy);

    if (!gameObjectManager)
    {
        ImGui::TextUnformatted("GameObjectManager is null.");
        ImGui::End();
        return;
    }

    const auto rootObjects = gameObjectManager->GetRootGameObjects();

    for (const auto& root : rootObjects)
    {
        DrawGameObjectNode(root);
    }

    ImGui::End();
}

void SceneHierarchyPanel::DrawGameObjectNode(const std::shared_ptr<QEGameObject>& gameObject)
{
    if (!gameObject)
    {
        return;
    }

    const bool isSelected = (editorContext->SelectedGameObjectId == gameObject->ID());

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;

    if (gameObject->childs.empty())
    {
        flags |= ImGuiTreeNodeFlags_Leaf;
    }

    if (isSelected)
    {
        flags |= ImGuiTreeNodeFlags_Selected;
    }

    const bool isOpen = ImGui::TreeNodeEx(
        gameObject->ID().c_str(),
        flags,
        "%s",
        gameObject->Name.c_str());

    if (ImGui::IsItemClicked())
    {
        editorContext->SelectedGameObjectId = gameObject->ID();
    }

    if (isOpen)
    {
        for (const auto& child : gameObject->childs)
        {
            DrawGameObjectNode(child);
        }

        ImGui::TreePop();
    }
}
