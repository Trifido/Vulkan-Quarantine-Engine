#pragma once

#include "IEditorPanel.h"

class EditorViewportResources;
struct EditorContext;

class ViewportPanel : public IEditorPanel
{
public:
    ViewportPanel(EditorContext* editorContext, EditorViewportResources* viewportResources);

    const char* GetName() const override { return "Viewport"; }
    void Draw() override;

private:
    const uint32_t renderWidth = 1280;
    const uint32_t renderHeight = 720;

    EditorContext* editorContext = nullptr;
    EditorViewportResources* viewportResources = nullptr;
};
