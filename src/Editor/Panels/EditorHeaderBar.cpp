#include "EditorHeaderBar.h"

#include <imgui.h>
#include <Editor/Core/EditorContext.h>
#include <Editor/Commands/EditorCommandManager.h>
#include <QESessionManager.h>
#include <QECamera.h>
#include <QECameraController.h>
#include <QEGameObject.h>

namespace
{
    static constexpr const char* kEditorCameraPopupId = "EditorCameraSettingsPopup";
}

EditorHeaderBar::EditorHeaderBar(
    EditorContext* editorContext,
    EditorCommandManager* commandManager,
    QESessionManager* sessionManager)
    : editorContext(editorContext)
    , commandManager(commandManager)
    , sessionManager(sessionManager)
{
}

void EditorHeaderBar::SetOnSaveRequested(const std::function<void()>& callback)
{
    onSaveRequested = callback;
}

void EditorHeaderBar::Draw()
{
    if (!ImGui::BeginMenuBar())
    {
        return;
    }

    DrawFileMenu();
    DrawWindowMenu();
    DrawDebugMenu();
    DrawCameraSetup();

    ImGui::EndMenuBar();
}

void EditorHeaderBar::DrawFileMenu()
{
    if (ImGui::BeginMenu("File"))
    {
        if (ImGui::MenuItem("Save", "Ctrl+S"))
        {
            if (onSaveRequested)
            {
                onSaveRequested();
            }
        }

        ImGui::EndMenu();
    }
}

void EditorHeaderBar::DrawWindowMenu()
{
    if (!editorContext)
    {
        return;
    }

    if (ImGui::BeginMenu("Window"))
    {
        ImGui::MenuItem("Hierarchy", nullptr, &editorContext->ShowHierarchy);
        ImGui::MenuItem("Inspector", nullptr, &editorContext->ShowInspector);
        ImGui::MenuItem("Viewport", nullptr, &editorContext->ShowViewport);
        ImGui::MenuItem("Console", nullptr, &editorContext->ShowConsole);
        ImGui::MenuItem("Content Browser", nullptr, &editorContext->ShowContentBrowser);
        ImGui::MenuItem("ImGui Demo", nullptr, &editorContext->ShowDemoWindow);
        ImGui::EndMenu();
    }
}

void EditorHeaderBar::DrawDebugMenu()
{
    if (!editorContext || !sessionManager)
    {
        return;
    }

    if (ImGui::BeginMenu("Debug"))
    {
        const bool showColliders = sessionManager->ShowColliderDebug();
        if (ImGui::MenuItem("Colliders", nullptr, showColliders))
        {
            sessionManager->SetShowColliderDebug(!showColliders);
        }

        const bool showAABBs = sessionManager->ShowCullingAABBDebug();
        if (ImGui::MenuItem("AABB Culling", nullptr, showAABBs))
        {
            sessionManager->SetShowCullingAABBDebug(!showAABBs);
        }

        ImGui::Separator();

        ImGui::MenuItem("Console Logs", nullptr, &editorContext->ShowConsole);

        ImGui::EndMenu();
    }
}

void EditorHeaderBar::DrawCameraSetup()
{
    DrawEditorCameraButton();
}

void EditorHeaderBar::DrawEditorCameraButton()
{
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetStyleColorVec4(ImGuiCol_HeaderHovered));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImGui::GetStyleColorVec4(ImGuiCol_HeaderActive));

    if (ImGui::Button("Editor Camera"))
    {
        ImGui::OpenPopup(kEditorCameraPopupId);
    }

    ImGui::PopStyleColor(3);

    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("Editor camera settings");
    }

    DrawEditorCameraPopup();
}

void EditorHeaderBar::DrawEditorCameraPopup()
{
    auto editorCamera = sessionManager ? sessionManager->EditorCamera() : nullptr;
    auto editorCameraOwner = editorCamera ? editorCamera->Owner : nullptr;
    auto controller = editorCameraOwner ? editorCameraOwner->GetComponent<QECameraController>() : nullptr;

    if (ImGui::BeginPopup(kEditorCameraPopupId))
    {
        ImGui::TextUnformatted("Editor Camera");
        ImGui::Separator();

        if (!editorCamera)
        {
            ImGui::TextUnformatted("Editor camera not found.");
        }
        else
        {
            float nearValue = editorCamera->GetNear();
            float farValue = editorCamera->GetFar();
            float fovValue = editorCamera->GetFOV();

            if (ImGui::DragFloat("Near", &nearValue, 0.01f, 0.001f, 1000.0f, "%.3f"))
            {
                editorCamera->SetNear(nearValue);
            }

            if (ImGui::DragFloat("Far", &farValue, 1.0f, 0.1f, 50000.0f, "%.1f"))
            {
                editorCamera->SetFar(farValue);
            }

            if (ImGui::DragFloat("FOV", &fovValue, 0.1f, 1.0f, 179.0f, "%.1f"))
            {
                editorCamera->SetFOV(fovValue);
            }
        }

        ImGui::Separator();

        if (!controller)
        {
            ImGui::TextUnformatted("Editor camera controller not found.");
        }
        else
        {
            ImGui::DragFloat("Speed", &controller->Speed, 0.1f, 0.01f, 1000.0f, "%.2f");
            ImGui::DragFloat("Mouse Sensitivity", &controller->MouseSensitivity, 0.001f, 0.001f, 10.0f, "%.3f");
        }

        ImGui::EndPopup();
    }
}
