#pragma once

#include <cstdint>

struct EditorContext
{
    bool IsPlaying = false;
    bool IsPaused = false;

    bool ShowHierarchy = true;
    bool ShowInspector = true;
    bool ShowViewport = true;
    bool ShowConsole = true;
    bool ShowContentBrowser = true;
    bool ShowDemoWindow = false;

    uint32_t ViewportWidth = 1;
    uint32_t ViewportHeight = 1;

    bool ViewportFocused = false;
    bool ViewportHovered = false;
    bool ViewportImageHovered = false;

    float ViewportScreenX = 0.0f;
    float ViewportScreenY = 0.0f;
    float ViewportScreenWidth = 0.0f;
    float ViewportScreenHeight = 0.0f;
};
