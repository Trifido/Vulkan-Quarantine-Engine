#pragma once

#include "IEditorPanel.h"
#include <filesystem>
#include <string>
#include <imgui.h>

class QETexturePreview;

class QETextureViewerPanel : public IEditorPanel
{
public:
    explicit QETextureViewerPanel(const std::filesystem::path& texturePath);
    ~QETextureViewerPanel() override;

    void Draw() override;
    const char* GetName() const override { return _windowTitle.c_str(); }

    bool IsOpen() const { return _isOpen; }
    const std::filesystem::path& GetTexturePath() const { return _texturePath; }

private:
    ImVec2 ComputeFitSize(const ImVec2& avail, int texWidth, int texHeight) const;
    std::string BuildWindowTitle() const;

private:
    std::filesystem::path _texturePath;
    std::string _windowTitle;
    bool _isOpen = true;
    bool _loaded = false;
    bool _loadAttempted = false;

    float _zoom = 1.0f;
    bool _fitToWindow = true;

    std::unique_ptr<QETexturePreview> _preview;
};
