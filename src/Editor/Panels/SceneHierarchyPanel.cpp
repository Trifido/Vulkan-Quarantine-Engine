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

    dragDropHandledThisFrame = false;

    HandleDeleteShortcut();
    DrawWindowContextMenu();
    DrawRenamePopup();

    const auto rootObjects = gameObjectManager->GetRootGameObjects();

    for (const auto& root : rootObjects)
    {
        DrawGameObjectNode(root);
    }

    if (draggingGameObject && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
    {
        if (!dragDropHandledThisFrame && ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows))
        {
            ReparentGameObject(draggingGameObject, nullptr);
        }

        draggingGameObject.reset();
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
        {
            selectionManager->SelectGameObject(gameObject);
        }
    }

    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
    {
        StartRename(gameObject);
    }

    if (isSelected &&
        ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) &&
        ImGui::IsKeyPressed(ImGuiKey_F2))
    {
        StartRename(gameObject);
    }

    if (ImGui::BeginPopupContextItem())
    {
        if (selectionManager)
        {
            selectionManager->SelectGameObject(gameObject);
        }

        if (ImGui::MenuItem("Rename"))
        {
            StartRename(gameObject);
        }

        if (ImGui::MenuItem("Delete"))
        {
            pendingDeleteGameObject = gameObject;
        }

        ImGui::EndPopup();
    }

    if (ImGui::BeginDragDropSource())
    {
        draggingGameObject = gameObject;

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
                        if (ReparentGameObject(dragged, gameObject))
                        {
                            dragDropHandledThisFrame = true;
                        }
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

void SceneHierarchyPanel::StartRename(const std::shared_ptr<QEGameObject>& gameObject)
{
    if (!gameObject)
        return;

    if (gameObject->Name == "QECameraEditor")
        return;

    renamingGameObject = gameObject;
    renameBuffer = gameObject->Name;
    openRenamePopup = true;
}

void SceneHierarchyPanel::DrawRenamePopup()
{
    if (openRenamePopup)
    {
        ImGui::OpenPopup("Rename GameObject");
        openRenamePopup = false;
    }

    if (!renamingGameObject)
        return;

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    const ImVec2 center = viewport->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(360.0f, 0.0f), ImGuiCond_Appearing);

    if (ImGui::BeginPopupModal("Rename GameObject", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        static bool focusRenameField = true;

        char buffer[256];
        memset(buffer, 0, sizeof(buffer));
        strncpy_s(buffer, sizeof(buffer), renameBuffer.c_str(), _TRUNCATE);

        ImGui::TextUnformatted("New name");

        if (focusRenameField)
        {
            ImGui::SetKeyboardFocusHere();
            focusRenameField = false;
        }

        const bool enterPressed = ImGui::InputText(
            "##RenameGameObject",
            buffer,
            sizeof(buffer),
            ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);

        renameBuffer = buffer;

        if (ImGui::IsKeyPressed(ImGuiKey_Escape))
        {
            renamingGameObject.reset();
            renameBuffer.clear();
            focusRenameField = true;
            ImGui::CloseCurrentPopup();
        }

        if (enterPressed)
        {
            if (!renameBuffer.empty())
            {
                gameObjectManager->RenameGameObject(renamingGameObject, renameBuffer);
            }

            renamingGameObject.reset();
            renameBuffer.clear();
            focusRenameField = true;
            ImGui::CloseCurrentPopup();
        }

        if (ImGui::Button("OK", ImVec2(120.0f, 0.0f)))
        {
            if (!renameBuffer.empty())
            {
                gameObjectManager->RenameGameObject(renamingGameObject, renameBuffer);
            }

            renamingGameObject.reset();
            renameBuffer.clear();
            focusRenameField = true;
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();

        if (ImGui::Button("Cancel", ImVec2(120.0f, 0.0f)))
        {
            renamingGameObject.reset();
            renameBuffer.clear();
            focusRenameField = true;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}
