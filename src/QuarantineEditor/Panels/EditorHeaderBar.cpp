#include "EditorHeaderBar.h"

#include <imgui.h>
#include <QuarantineEditor/Core/EditorContext.h>
#include <QuarantineEditor/Commands/EditorCommandManager.h>
#include <QuarantineEditor/Core/QEEditorCameraController.h>
#include <QECamera.h>
#include <PhysicsModule.h>
#include <QEGameObject.h>

namespace
{
    static constexpr const char* kEditorCameraPopupId = "EditorCameraSettingsPopup";
    static constexpr const char* kPhysicsSettingsPopupId = "PhysicsSettingsPopup";
}

EditorHeaderBar::EditorHeaderBar(
    EditorContext* editorContext,
    EditorCommandManager* commandManager,
    std::function<std::shared_ptr<QECamera>()> getEditorCamera,
    std::function<bool()> getShowEditorGrid,
    std::function<void(bool)> setShowEditorGrid,
    std::function<bool()> getShowColliderDebug,
    std::function<void(bool)> setShowColliderDebug,
    std::function<bool()> getShowCullingAABBDebug,
    std::function<void(bool)> setShowCullingAABBDebug,
    PhysicsModule* physicsModule)
    : editorContext(editorContext)
    , commandManager(commandManager)
    , getEditorCamera(std::move(getEditorCamera))
    , getShowEditorGrid(std::move(getShowEditorGrid))
    , setShowEditorGrid(std::move(setShowEditorGrid))
    , getShowColliderDebug(std::move(getShowColliderDebug))
    , setShowColliderDebug(std::move(setShowColliderDebug))
    , getShowCullingAABBDebug(std::move(getShowCullingAABBDebug))
    , setShowCullingAABBDebug(std::move(setShowCullingAABBDebug))
    , physicsModule(physicsModule)
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
        ImGui::MenuItem("Material Inspector", nullptr, &editorContext->ShowMaterialInspector);
        ImGui::MenuItem("Animation Graph", nullptr, &editorContext->ShowAnimationGraph);
        ImGui::MenuItem("Viewport", nullptr, &editorContext->ShowViewport);
        ImGui::MenuItem("Console", nullptr, &editorContext->ShowConsole);
        ImGui::MenuItem("Content Browser", nullptr, &editorContext->ShowContentBrowser);
        ImGui::MenuItem("ImGui Demo", nullptr, &editorContext->ShowDemoWindow);
        ImGui::EndMenu();
    }
}

void EditorHeaderBar::DrawDebugMenu()
{
    if (!editorContext)
    {
        return;
    }

    if (ImGui::BeginMenu("Debug"))
    {
        const bool showGrid = getShowEditorGrid ? getShowEditorGrid() : false;
        if (ImGui::MenuItem("Grid", nullptr, showGrid))
        {
            if (setShowEditorGrid)
            {
                setShowEditorGrid(!showGrid);
            }
        }

        const bool showColliders = getShowColliderDebug ? getShowColliderDebug() : false;
        if (ImGui::MenuItem("Colliders", nullptr, showColliders))
        {
            if (setShowColliderDebug)
            {
                setShowColliderDebug(!showColliders);
            }
        }

        const bool showAABBs = getShowCullingAABBDebug ? getShowCullingAABBDebug() : false;
        if (ImGui::MenuItem("AABB Culling", nullptr, showAABBs))
        {
            if (setShowCullingAABBDebug)
            {
                setShowCullingAABBDebug(!showAABBs);
            }
        }

        ImGui::Separator();

        ImGui::MenuItem("Console Logs", nullptr, &editorContext->ShowConsole);

        ImGui::EndMenu();
    }
}

void EditorHeaderBar::DrawCameraSetup()
{
    DrawEditorCameraButton();
    ImGui::SameLine();
    DrawPhysicsSettingsButton();
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
    auto editorCamera = getEditorCamera ? getEditorCamera() : nullptr;
    auto editorCameraOwner = editorCamera ? editorCamera->Owner : nullptr;
    auto controller = editorCameraOwner ? editorCameraOwner->GetComponent<QEEditorCameraController>() : nullptr;

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

void EditorHeaderBar::DrawPhysicsSettingsButton()
{
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetStyleColorVec4(ImGuiCol_HeaderHovered));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImGui::GetStyleColorVec4(ImGuiCol_HeaderActive));

    if (ImGui::Button("Physics"))
    {
        ImGui::OpenPopup(kPhysicsSettingsPopupId);
    }

    ImGui::PopStyleColor(3);

    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("Physics settings");
    }

    DrawPhysicsSettingsPopup();
}

void EditorHeaderBar::DrawPhysicsSettingsPopup()
{
    if (ImGui::BeginPopup(kPhysicsSettingsPopupId))
    {
        ImGui::TextUnformatted("Physics Settings");
        ImGui::Separator();

        if (!physicsModule)
        {
            ImGui::TextUnformatted("PhysicsModule is null.");
        }
        else
        {
            float gravity = physicsModule->GetGravity();
            if (ImGui::DragFloat("Gravity", &gravity, 0.1f, -200.0f, 200.0f, "%.2f"))
            {
                physicsModule->SetGravity(gravity);
            }

            ImGui::Text("Tracked Bodies: %d", static_cast<int>(physicsModule->GetTrackedBodyCount()));
        }

        ImGui::Separator();

        if (!getShowColliderDebug || !setShowColliderDebug)
        {
            ImGui::TextUnformatted("Collider debug controls unavailable.");
        }
        else
        {
            bool showColliders = getShowColliderDebug();
            if (ImGui::Checkbox("Show Collider Debug", &showColliders))
            {
                setShowColliderDebug(showColliders);
            }
        }

        ImGui::EndPopup();
    }
}
