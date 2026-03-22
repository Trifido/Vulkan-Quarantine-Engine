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

    // OJO:
    // Aquí NO usamos el tamaño del panel para recrear el render target
    // si quieres mantener un render lógico fijo tipo game/editor view.
    //
    // Usa aquí la resolución fija que estés utilizando realmente
    // para el render offscreen.
    //
    // Ejemplo:
    const uint32_t renderWidth = 1280;
    const uint32_t renderHeight = 720;

    editorContext->ViewportWidth = renderWidth;
    editorContext->ViewportHeight = renderHeight;

    if (viewportResources && viewportResources->NeedsResize(renderWidth, renderHeight))
    {
        viewportResources->Resize(renderWidth, renderHeight);
    }

    if (viewportResources && viewportResources->IsValid())
    {
        const float texW = static_cast<float>(renderWidth);
        const float texH = static_cast<float>(renderHeight);

        // COVER:
        // escalamos para cubrir completamente el panel
        const float scaleX = avail.x / texW;
        const float scaleY = avail.y / texH;
        const float scale = std::max(scaleX, scaleY);

        const ImVec2 imageSize(texW * scale, texH * scale);

        // rectángulo visible del panel
        const ImVec2 panelMin = ImGui::GetCursorScreenPos();
        const ImVec2 panelMax(panelMin.x + avail.x, panelMin.y + avail.y);

        // centramos la imagen escalada respecto al panel
        const ImVec2 imageMin(
            panelMin.x + (avail.x - imageSize.x) * 0.5f,
            panelMin.y + (avail.y - imageSize.y) * 0.5f
        );
        const ImVec2 imageMax(
            imageMin.x + imageSize.x,
            imageMin.y + imageSize.y
        );

        // Reservamos espacio en layout para que ImGui trate esta zona
        // como ocupada por el viewport.
        ImGui::InvisibleButton("ViewportImage", avail);

        ImDrawList* drawList = ImGui::GetWindowDrawList();
        drawList->PushClipRect(panelMin, panelMax, true);
        drawList->AddImage(
            viewportResources->GetImGuiTexture(),
            imageMin,
            imageMax,
            ImVec2(0, 0),
            ImVec2(1, 1)
        );
        drawList->PopClipRect();
    }
    else
    {
        ImGui::TextUnformatted("Viewport not initialized.");
    }

    ImGui::End();
    ImGui::PopStyleVar();
}
