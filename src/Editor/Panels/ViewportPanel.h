#pragma once
#include "IEditorPanel.h"

struct EditorContext;

class ViewportPanel : public IEditorPanel
{
public:
    ViewportPanel(EditorContext* context);

    const char* GetName() const override { return "Viewport"; }
    void Draw() override;

private:
    EditorContext* context = nullptr;
};
