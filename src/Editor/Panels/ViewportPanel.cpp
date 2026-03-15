#include "ViewportPanel.h"

#include <imgui.h>
#include <Editor/Core/EditorContext.h>
#include <Editor/Rendering/EditorViewportResources.h>

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

    ImGui::Begin("Viewport", &editorContext->ShowViewport);

    editorContext->ViewportFocused = ImGui::IsWindowFocused();
    editorContext->ViewportHovered = ImGui::IsWindowHovered();

    ImVec2 available = ImGui::GetContentRegionAvail();

    uint32_t width = static_cast<uint32_t>(available.x > 1.0f ? available.x : 1.0f);
    uint32_t height = static_cast<uint32_t>(available.y > 1.0f ? available.y : 1.0f);

    editorContext->ViewportWidth = width;
    editorContext->ViewportHeight = height;

    if (viewportResources && viewportResources->NeedsResize(width, height))
    {
        viewportResources->Resize(width, height);
    }

    if (viewportResources && viewportResources->IsValid())
    {
        ImGui::Image(viewportResources->GetImGuiTexture(), available);
    }
    else
    {
        ImGui::TextUnformatted("Viewport not initialized.");
    }

    ImGui::End();
}
