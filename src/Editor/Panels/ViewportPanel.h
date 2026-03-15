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
    EditorContext* editorContext = nullptr;
    EditorViewportResources* viewportResources = nullptr;
};
