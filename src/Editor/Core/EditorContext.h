#pragma once

#include <string>

enum class QEGizmoOperation
{
    None = 0,
    Translate,
    Rotate,
    Scale
};

struct EditorContext
{
    std::string SelectedGameObjectId;

    bool IsPlaying = false;
    bool IsPaused = false;

    bool ShowHierarchy = true;
    bool ShowInspector = true;
    bool ShowViewport = true;
    bool ShowConsole = true;
    bool ShowContentBrowser = true;
    bool ShowDemoWindow = false;

    QEGizmoOperation CurrentGizmoOperation = QEGizmoOperation::Translate;

    uint32_t ViewportWidth = 1;
    uint32_t ViewportHeight = 1;
    bool ViewportFocused = false;
    bool ViewportHovered = false;
};
