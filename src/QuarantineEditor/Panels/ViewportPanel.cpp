#include "ViewportPanel.h"

#include <imgui.h>
#include <QESessionManager.h>
#include <QuarantineEditor/Core/EditorContext.h>
#include <QuarantineEditor/Core/EditorSelectionManager.h>
#include <QuarantineEditor/Core/QEGizmoController.h>
#include <QuarantineEditor/Commands/EditorTransformUtils.h>
#include <QuarantineEditor/Rendering/EditorViewportResources.h>
#include <QECamera.h>
#include <GameObjectManager.h>

namespace
{
    bool IsEditorInputBlocked()
    {
        return
            ImGui::IsPopupOpen("AddComponentPopup", ImGuiPopupFlags_AnyPopupId) ||
            ImGui::IsPopupOpen("Rename GameObject", ImGuiPopupFlags_AnyPopupId);
    }
}

ViewportPanel::ViewportPanel(
    EditorContext* editorContext,
    EditorViewportResources* viewportResources,
    EditorSelectionManager* selectionManager,
    QEGizmoController* gizmoController,
    EditorPickingSystem* pickingSystem,
    EditorCommandManager* commandManager)
    : editorContext(editorContext)
    , viewportResources(viewportResources)
    , selectionManager(selectionManager)
    , gizmoController(gizmoController)
    , pickingSystem(pickingSystem)
    , commandManager(commandManager)
{
}

void ViewportPanel::Draw()
{
    if (!editorContext || !editorContext->ShowViewport)
    {
        return;
    }

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("Viewport", &editorContext->ShowViewport);

    const bool inputBlocked = IsEditorInputBlocked();

    editorContext->BlockViewportInput = inputBlocked;
    editorContext->ViewportFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
    editorContext->ViewportHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows);
    editorContext->ViewportImageHovered = false;
    editorContext->EditorCameraInputEnabled = false;

    ImVec2 avail = ImGui::GetContentRegionAvail();
    avail.x = (avail.x > 1.0f) ? avail.x : 1.0f;
    avail.y = (avail.y > 1.0f) ? avail.y : 1.0f;

    const uint32_t renderWidth = std::max(1u, static_cast<uint32_t>(avail.x));
    const uint32_t renderHeight = std::max(1u, static_cast<uint32_t>(avail.y));

    editorContext->ViewportWidth = renderWidth;
    editorContext->ViewportHeight = renderHeight;

    if (viewportResources && viewportResources->NeedsResize(renderWidth, renderHeight))
    {
        viewportResources->Resize(renderWidth, renderHeight);

        auto sessionManager = QESessionManager::getInstance();
        sessionManager->UpdateEditorCameraViewportSize(renderWidth, renderHeight);
    }

    if (viewportResources && viewportResources->IsValid())
    {
        ImVec2 imagePos = ImGui::GetCursorScreenPos();
        ImVec2 imageSize((float)renderWidth, (float)renderHeight);

        editorContext->ViewportScreenX = imagePos.x;
        editorContext->ViewportScreenY = imagePos.y;
        editorContext->ViewportScreenWidth = imageSize.x;
        editorContext->ViewportScreenHeight = imageSize.y;

        ImGui::Image(viewportResources->GetImGuiTexture(), imageSize);

        if (!inputBlocked)
        {
            HandleAssetDropTarget();

            ImVec2 mousePos = ImGui::GetMousePos();
            const bool insideX = mousePos.x >= imagePos.x && mousePos.x <= (imagePos.x + imageSize.x);
            const bool insideY = mousePos.y >= imagePos.y && mousePos.y <= (imagePos.y + imageSize.y);
            editorContext->ViewportImageHovered = insideX && insideY;

            editorContext->EditorCameraInputEnabled =
                editorContext->ViewportImageHovered || editorContext->ViewportFocused;

            HandleViewportShortcuts();
            HandlePicking();
        }
        else
        {
            editorContext->ViewportImageHovered = false;
            editorContext->EditorCameraInputEnabled = false;
        }

        auto sessionManager = QESessionManager::getInstance();
        auto editorCamera = sessionManager->EditorCamera();

        if (!inputBlocked && selectionManager && gizmoController && editorCamera)
        {
            auto selectedObject = selectionManager->GetSelectedGameObject();
            if (selectedObject)
            {
                gizmoController->Draw(
                    selectedObject,
                    editorCamera,
                    glm::vec2(editorContext->ViewportScreenX, editorContext->ViewportScreenY),
                    glm::vec2(editorContext->ViewportScreenWidth, editorContext->ViewportScreenHeight));
            }
        }

        if (!inputBlocked)
        {
            HandleGizmoCommandTracking();
        }
        else
        {
            wasUsingGizmoLastFrame = false;
            gizmoTrackedObjectId.clear();
        }
    }
    else
    {
        editorContext->ViewportScreenX = 0.0f;
        editorContext->ViewportScreenY = 0.0f;
        editorContext->ViewportScreenWidth = 0.0f;
        editorContext->ViewportScreenHeight = 0.0f;
        editorContext->ViewportImageHovered = false;
        wasUsingGizmoLastFrame = false;
        gizmoTrackedObjectId.clear();

        ImGui::TextUnformatted("Viewport not initialized.");
    }

    ImGui::End();
    ImGui::PopStyleVar();
}

void ViewportPanel::HandleViewportShortcuts()
{
    if (!editorContext || !gizmoController)
        return;

    if (!editorContext->ViewportFocused && !editorContext->ViewportImageHovered)
        return;

    if (ImGui::IsKeyPressed(ImGuiKey_Q, false))
    {
        gizmoController->SetOperation(QEGizmoController::Operation::None);
    }

    if (ImGui::IsKeyPressed(ImGuiKey_T, false))
    {
        gizmoController->SetOperation(QEGizmoController::Operation::Translate);
    }

    if (ImGui::IsKeyPressed(ImGuiKey_R, false))
    {
        gizmoController->SetOperation(QEGizmoController::Operation::Rotate);
    }

    if (ImGui::IsKeyPressed(ImGuiKey_E, false))
    {
        gizmoController->SetOperation(QEGizmoController::Operation::Scale);
    }
}

void ViewportPanel::HandlePicking()
{
    if (!editorContext || !selectionManager || !pickingSystem || !gizmoController)
        return;

    if (!editorContext->ViewportImageHovered)
        return;

    if (!ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        return;

    if (gizmoController->IsOver() || gizmoController->IsUsing())
        return;

    auto editorCamera = QESessionManager::getInstance()->EditorCamera();
    if (!editorCamera || !editorCamera->CameraData)
        return;

    ImVec2 mousePos = ImGui::GetMousePos();

    auto pickedObject = pickingSystem->PickGameObject(
        editorCamera,
        mousePos.x,
        mousePos.y,
        editorContext->ViewportScreenX,
        editorContext->ViewportScreenY,
        editorContext->ViewportScreenWidth,
        editorContext->ViewportScreenHeight);

    if (pickedObject)
    {
        selectionManager->SelectGameObject(pickedObject);
    }
    else
    {
        selectionManager->ClearSelection();
    }
}

void ViewportPanel::HandleGizmoCommandTracking()
{
    if (!selectionManager || !gizmoController || !commandManager)
        return;

    auto selectedObject = selectionManager->GetSelectedGameObject();
    if (!selectedObject)
    {
        wasUsingGizmoLastFrame = false;
        gizmoTrackedObjectId.clear();
        return;
    }

    auto transform = selectedObject->GetComponent<QETransform>();
    if (!transform)
    {
        wasUsingGizmoLastFrame = false;
        gizmoTrackedObjectId.clear();
        return;
    }

    const bool isUsingNow = gizmoController->IsUsing();
    const std::string currentObjectId = selectedObject->ID();

    // Inicio del drag del gizmo
    if (!wasUsingGizmoLastFrame && isUsingNow)
    {
        gizmoTrackedObjectId = currentObjectId;
        gizmoBeginState = EditorTransformUtils::CaptureState(transform);
    }

    // Fin del drag del gizmo
    if (wasUsingGizmoLastFrame && !isUsingNow)
    {
        if (!gizmoTrackedObjectId.empty())
        {
            auto trackedObject = selectionManager->GetSelectedGameObject();

            if (!trackedObject || trackedObject->ID() != gizmoTrackedObjectId)
            {
                trackedObject = GameObjectManager::getInstance()->GetGameObjectById(gizmoTrackedObjectId);
            }

            if (trackedObject)
            {
                auto trackedTransform = trackedObject->GetComponent<QETransform>();
                if (trackedTransform)
                {
                    TransformState endState = EditorTransformUtils::CaptureState(trackedTransform);

                    const bool changed =
                        gizmoBeginState.Position != endState.Position ||
                        gizmoBeginState.Rotation != endState.Rotation ||
                        gizmoBeginState.Scale != endState.Scale;

                    if (changed)
                    {
                        commandManager->PushExecutedCommand(
                            std::make_unique<TransformCommand>(
                                gizmoTrackedObjectId,
                                gizmoBeginState,
                                endState));
                    }
                }
            }
        }

        gizmoTrackedObjectId.clear();
    }

    wasUsingGizmoLastFrame = isUsingNow;
}

void ViewportPanel::HandleAssetDropTarget()
{
    if (!editorContext)
        return;

    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* peekPayload =
            ImGui::AcceptDragDropPayload("QE_PROJECT_ASSET_PATH", ImGuiDragDropFlags_AcceptPeekOnly))
        {
            ImDrawList* drawList = ImGui::GetWindowDrawList();
            ImVec2 min = ImGui::GetItemRectMin();
            ImVec2 max = ImGui::GetItemRectMax();

            drawList->AddRect(min, max, IM_COL32(120, 180, 255, 255), 0.0f, 0, 2.0f);
        }

        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("QE_PROJECT_ASSET_PATH"))
        {
            if (payload->Data && payload->DataSize > 0)
            {
                const char* droppedPath = static_cast<const char*>(payload->Data);
                HandleDroppedAssetPath(std::string(droppedPath));
            }
        }

        ImGui::EndDragDropTarget();
    }
}

void ViewportPanel::HandleDroppedAssetPath(const std::string& assetPath)
{
    if (assetPath.empty())
        return;

    if (OnAssetDroppedInViewport)
    {
        OnAssetDroppedInViewport(assetPath);
    }
}
