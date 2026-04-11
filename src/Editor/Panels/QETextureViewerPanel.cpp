#include "QETextureViewerPanel.h"

#include "Editor/Rendering/QETexturePreview.h"
#include <imgui.h>
#include <algorithm>

QETextureViewerPanel::QETextureViewerPanel(const std::filesystem::path& texturePath)
    : _texturePath(texturePath),
    _preview(std::make_unique<QETexturePreview>())
{
    _windowTitle = BuildWindowTitle();
}

QETextureViewerPanel::~QETextureViewerPanel() = default;

std::string QETextureViewerPanel::BuildWindowTitle() const
{
    return "Texture Viewer - " + _texturePath.filename().string() + "##" + _texturePath.string();
}

ImVec2 QETextureViewerPanel::ComputeFitSize(const ImVec2& avail, int texWidth, int texHeight) const
{
    if (texWidth <= 0 || texHeight <= 0)
        return ImVec2(0.0f, 0.0f);

    const float scaleX = avail.x / static_cast<float>(texWidth);
    const float scaleY = avail.y / static_cast<float>(texHeight);
    const float scale = std::min(scaleX, scaleY);

    return ImVec2(
        static_cast<float>(texWidth) * scale,
        static_cast<float>(texHeight) * scale);
}

void QETextureViewerPanel::Draw()
{
    if (!_isOpen)
        return;

    if (!ImGui::Begin(_windowTitle.c_str(), &_isOpen))
    {
        ImGui::End();
        return;
    }

    if (!_loadAttempted)
    {
        _loadAttempted = true;
        _loaded = _preview->LoadFromFile(_texturePath.string());
    }

    if (!_loaded || !_preview->IsValid())
    {
        ImGui::TextUnformatted("Failed to load texture.");
        ImGui::TextWrapped("%s", _texturePath.string().c_str());
        ImGui::End();
        return;
    }

    ImGui::Text("Path: %s", _texturePath.filename().string().c_str());
    ImGui::Text("Size: %d x %d", _preview->GetWidth(), _preview->GetHeight());
    ImGui::Text("Mip Levels: %u", _preview->GetMipLevels());

    ImGui::Separator();

    if (ImGui::Button("Fit"))
    {
        _fitToWindow = true;
    }

    ImGui::SameLine();

    if (ImGui::Button("1:1"))
    {
        _fitToWindow = false;
        _zoom = 1.0f;
    }

    ImGui::SameLine();
    ImGui::SliderFloat("Zoom", &_zoom, 0.1f, 8.0f);

    ImGui::Separator();

    ImVec2 avail = ImGui::GetContentRegionAvail();

    ImVec2 imageSize;
    if (_fitToWindow)
    {
        imageSize = ComputeFitSize(avail, _preview->GetWidth(), _preview->GetHeight());
    }
    else
    {
        imageSize = ImVec2(
            static_cast<float>(_preview->GetWidth()) * _zoom,
            static_cast<float>(_preview->GetHeight()) * _zoom);
    }

    if (imageSize.x > 0.0f && imageSize.y > 0.0f)
    {
        ImGui::BeginChild("TextureScrollArea", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

        ImGui::Image(_preview->GetImGuiTexture(), imageSize);

        ImGui::EndChild();
    }

    ImGui::End();
}
