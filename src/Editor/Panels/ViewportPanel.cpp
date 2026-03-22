#include "ViewportPanel.h"

#include <imgui.h>
#include <QESessionManager.h>
#include <Editor/Core/EditorContext.h>
#include <Editor/Core/EditorSelectionManager.h>
#include <Editor/Core/QEGizmoController.h>
#include <Editor/Rendering/EditorViewportResources.h>

ViewportPanel::ViewportPanel(
    EditorContext* editorContext,
    EditorViewportResources* viewportResources,
    EditorSelectionManager* selectionManager,
    QEGizmoController* gizmoController)
    : editorContext(editorContext)
    , viewportResources(viewportResources)
    , selectionManager(selectionManager)
    , gizmoController(gizmoController)
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

    editorContext->ViewportFocused = ImGui::IsWindowFocused();
    editorContext->ViewportHovered = ImGui::IsWindowHovered();
    editorContext->ViewportImageHovered = false;

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

        ImVec2 mousePos = ImGui::GetMousePos();
        const bool insideX = mousePos.x >= imagePos.x && mousePos.x <= (imagePos.x + imageSize.x);
        const bool insideY = mousePos.y >= imagePos.y && mousePos.y <= (imagePos.y + imageSize.y);
        editorContext->ViewportImageHovered = insideX && insideY;

        HandleViewportShortcuts();

        auto sessionManager = QESessionManager::getInstance();
        auto editorCamera = sessionManager->EditorCamera();

        if (selectionManager && gizmoController && editorCamera)
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
    }
    else
    {
        editorContext->ViewportScreenX = 0.0f;
        editorContext->ViewportScreenY = 0.0f;
        editorContext->ViewportScreenWidth = 0.0f;
        editorContext->ViewportScreenHeight = 0.0f;
        editorContext->ViewportImageHovered = false;

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
