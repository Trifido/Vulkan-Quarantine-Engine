#include "ViewportPanel.h"

#include <imgui.h>
#include <Editor/Core/EditorContext.h>
#include <Editor/Rendering/EditorViewportResources.h>
#include <QESessionManager.h>

ViewportPanel::ViewportPanel(EditorContext* editorContext, EditorViewportResources* viewportResources)
    : editorContext(editorContext), viewportResources(viewportResources)
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
        ImVec2 imageSize((float)renderWidth, (float)renderHeight);
        ImGui::Image(viewportResources->GetImGuiTexture(), imageSize);
    }
    else
    {
        ImGui::TextUnformatted("Viewport not initialized.");
    }

    ImGui::End();
    ImGui::PopStyleVar();
}
