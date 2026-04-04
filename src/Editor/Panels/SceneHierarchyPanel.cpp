#include "SceneHierarchyPanel.h"

#include <imgui.h>
#include <GameObjectManager.h>
#include <QEGameObject.h>
#include <Editor/Core/EditorContext.h>
#include <Editor/Core/EditorSelectionManager.h>

SceneHierarchyPanel::SceneHierarchyPanel(
    GameObjectManager* gameObjectManager,
    EditorContext* editorContext,
    EditorSelectionManager* selectionManager)
    : gameObjectManager(gameObjectManager)
    , editorContext(editorContext)
    , selectionManager(selectionManager)
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

    HandleDeleteShortcut();
    DrawWindowContextMenu();
    DrawRootDropTarget();

    const auto rootObjects = gameObjectManager->GetRootGameObjects();

    for (const auto& root : rootObjects)
    {
        DrawGameObjectNode(root);
    }

    if (pendingCreateEmptyGameObject)
    {
        auto created = gameObjectManager->CreateEmptyGameObject("Empty GameObject");
        if (selectionManager && created)
        {
            selectionManager->SelectGameObject(created);
        }
        pendingCreateEmptyGameObject = false;
    }

    if (pendingDeleteGameObject)
    {
        if (selectionManager && selectionManager->IsSelected(pendingDeleteGameObject))
        {
            selectionManager->SelectGameObject(nullptr);
        }

        gameObjectManager->RemoveGameObject(pendingDeleteGameObject);
        pendingDeleteGameObject.reset();
    }

    ImGui::End();
}

void SceneHierarchyPanel::DrawWindowContextMenu()
{
    if (ImGui::BeginPopupContextWindow("SceneHierarchyWindowContext", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
    {
        if (ImGui::MenuItem("Create Empty GameObject"))
        {
            pendingCreateEmptyGameObject = true;
        }

        ImGui::EndPopup();
    }
}

void SceneHierarchyPanel::HandleDeleteShortcut()
{
    if (!selectionManager)
        return;

    if (!ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows))
        return;

    if (!ImGui::IsKeyPressed(ImGuiKey_Delete, false))
        return;

    // Esto asume que tu EditorSelectionManager expone un getter del seleccionado actual.
    auto selected = selectionManager->GetSelectedGameObject();
    if (!selected)
        return;

    if (selected->Name == "QECameraEditor")
        return;

    pendingDeleteGameObject = selected;
}

void SceneHierarchyPanel::DrawGameObjectNode(const std::shared_ptr<QEGameObject>& gameObject)
{
    if (!gameObject)
    {
        return;
    }

    if (gameObject->Name == "QECameraEditor")
    {
        return;
    }

    const bool isSelected = selectionManager && selectionManager->IsSelected(gameObject);

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
        if (selectionManager)
            selectionManager->SelectGameObject(gameObject);
    }

    if (ImGui::BeginPopupContextItem())
    {
        if (selectionManager)
            selectionManager->SelectGameObject(gameObject);

        if (ImGui::MenuItem("Delete"))
        {
            pendingDeleteGameObject = gameObject;
        }

        if (ImGui::MenuItem("Unparent"))
        {
            ReparentGameObject(gameObject, nullptr);
        }

        if (ImGui::MenuItem("Create Empty Child"))
        {
            auto child = gameObjectManager->CreateEmptyGameObject("Empty GameObject");
            ReparentGameObject(child, gameObject);
            if (selectionManager)
                selectionManager->SelectGameObject(child);
        }

        ImGui::EndPopup();
    }

    if (ImGui::BeginDragDropSource())
    {
        QEGameObject* rawPtr = gameObject.get();
        ImGui::SetDragDropPayload("HIERARCHY_GAMEOBJECT", &rawPtr, sizeof(QEGameObject*));
        ImGui::Text("Move: %s", gameObject->Name.c_str());
        ImGui::EndDragDropSource();
    }

    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("HIERARCHY_GAMEOBJECT"))
        {
            if (payload->DataSize == sizeof(QEGameObject*))
            {
                QEGameObject* draggedRaw = *static_cast<QEGameObject* const*>(payload->Data);
                if (draggedRaw)
                {
                    auto dragged = gameObjectManager->GetGameObjectById(draggedRaw->ID());
                    if (dragged)
                    {
                        ReparentGameObject(dragged, gameObject);
                    }
                }
            }
        }

        ImGui::EndDragDropTarget();
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

bool SceneHierarchyPanel::IsDescendantOf(const std::shared_ptr<QEGameObject>& node, const std::shared_ptr<QEGameObject>& potentialAncestor) const
{
    if (!node || !potentialAncestor)
        return false;

    QEGameObject* current = node->parent;

    while (current)
    {
        if (current == potentialAncestor.get())
            return true;

        current = current->parent;
    }

    return false;
}

bool SceneHierarchyPanel::ReparentGameObject(const std::shared_ptr<QEGameObject>& child, const std::shared_ptr<QEGameObject>& newParent)
{
    if (!child)
        return false;

    if (child->Name == "QECameraEditor")
        return false;

    if (newParent && newParent->Name == "QECameraEditor")
        return false;

    if (newParent == child)
        return false;

    if (newParent && IsDescendantOf(newParent, child))
        return false;

    if (child->parent == newParent.get())
        return false;

    if (child->parent)
    {
        auto oldParentShared = gameObjectManager->GetGameObjectById(child->parent->ID());
        if (oldParentShared)
        {
            oldParentShared->RemoveChild(child);
        }
        else
        {
            child->parent->RemoveChild(child);
        }
    }

    if (newParent)
    {
        newParent->AddChild(child, true);
    }

    return true;
}

void SceneHierarchyPanel::DrawRootDropTarget()
{
    ImGui::Separator();
    ImGui::TextUnformatted("Drop here to unparent");

    ImVec2 avail = ImGui::GetContentRegionAvail();
    const float width = (avail.x > 1.0f) ? avail.x : 1.0f;
    const float height = 32.0f;

    ImVec2 cursor = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton("##HierarchyRootDropTarget", ImVec2(width, height));

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImU32 color = IM_COL32(90, 90, 90, 120);
    drawList->AddRect(cursor, ImVec2(cursor.x + width, cursor.y + height), color);
    drawList->AddText(ImVec2(cursor.x + 8.0f, cursor.y + 8.0f), IM_COL32(220, 220, 220, 255), "Root");

    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("HIERARCHY_GAMEOBJECT"))
        {
            if (payload->DataSize == sizeof(QEGameObject*))
            {
                QEGameObject* draggedRaw = *static_cast<QEGameObject* const*>(payload->Data);
                if (draggedRaw)
                {
                    auto dragged = gameObjectManager->GetGameObjectById(draggedRaw->ID());
                    if (dragged)
                    {
                        ReparentGameObject(dragged, nullptr);
                    }
                }
            }
        }

        ImGui::EndDragDropTarget();
    }
}
