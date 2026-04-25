#include "QETextureViewerPanel.h"

#include "QuarantineEditor/Rendering/QETexturePreview.h"
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

void QETextureViewerPanel::HandleCanvasInput(const ImVec2& canvasPos, const ImVec2& canvasSize)
{
    ImGuiIO& io = ImGui::GetIO();

    const bool hovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows);
    if (!hovered)
        return;

    if (io.MouseWheel != 0.0f)
    {
        _fitToWindow = false;

        const float prevZoom = _zoom;
        _zoom += io.MouseWheel * 0.01f;
        _zoom = std::clamp(_zoom, 0.1f, 8.0f);

        if (_zoom != prevZoom)
        {
            const ImVec2 mousePos = io.MousePos;
            const float relX = (mousePos.x - canvasPos.x + ImGui::GetScrollX()) / std::max(1.0f, canvasSize.x);
            const float relY = (mousePos.y - canvasPos.y + ImGui::GetScrollY()) / std::max(1.0f, canvasSize.y);

            const float zoomRatio = _zoom / prevZoom;

            ImGui::SetScrollX((ImGui::GetScrollX() + (mousePos.x - canvasPos.x)) * zoomRatio - (mousePos.x - canvasPos.x));
            ImGui::SetScrollY((ImGui::GetScrollY() + (mousePos.y - canvasPos.y)) * zoomRatio - (mousePos.y - canvasPos.y));
        }
    }

    if (ImGui::IsMouseClicked(ImGuiMouseButton_Middle))
    {
        _isPanning = true;
        _lastPanMousePos = io.MousePos;
    }

    if (_isPanning)
    {
        if (ImGui::IsMouseDown(ImGuiMouseButton_Middle))
        {
            const ImVec2 delta(
                io.MousePos.x - _lastPanMousePos.x,
                io.MousePos.y - _lastPanMousePos.y);

            ImGui::SetScrollX(ImGui::GetScrollX() - delta.x);
            ImGui::SetScrollY(ImGui::GetScrollY() - delta.y);

            _lastPanMousePos = io.MousePos;
        }
        else
        {
            _isPanning = false;
        }
    }
}

void QETextureViewerPanel::DrawTextureCanvas()
{
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

    if (imageSize.x <= 0.0f || imageSize.y <= 0.0f)
        return;

    if (ImGui::BeginChild(
        "TextureScrollArea",
        ImVec2(0, 0),
        false,
        ImGuiWindowFlags_HorizontalScrollbar))
    {
        const ImVec2 childPos = ImGui::GetCursorScreenPos();
        const ImVec2 childAvail = ImGui::GetContentRegionAvail();

        HandleCanvasInput(childPos, imageSize);

        float offsetX = 0.0f;
        float offsetY = 0.0f;

        if (imageSize.x < childAvail.x)
            offsetX = (childAvail.x - imageSize.x) * 0.5f;

        if (imageSize.y < childAvail.y)
            offsetY = (childAvail.y - imageSize.y) * 0.5f;

        if (offsetX > 0.0f)
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offsetX);

        if (offsetY > 0.0f)
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + offsetY);

        const ImVec2 imageScreenPos = ImGui::GetCursorScreenPos();

        if (_showCheckerboard)
        {
            DrawCheckerboard(imageScreenPos, imageSize);

            ImGui::GetWindowDrawList()->AddRect(
                imageScreenPos,
                ImVec2(imageScreenPos.x + imageSize.x, imageScreenPos.y + imageSize.y),
                IM_COL32(255, 0, 0, 255),
                0.0f,
                0,
                2.0f);
        }

        ImGui::Image(_preview->GetImGuiTexture(), imageSize);
    }

    ImGui::EndChild();
}

void QETextureViewerPanel::DrawCheckerboard(const ImVec2& minPos, const ImVec2& size) const
{
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    if (!drawList || size.x <= 0.0f || size.y <= 0.0f)
        return;

    const float cellSize = 12.0f;

    const ImU32 colorA = IM_COL32(110, 110, 110, 255);
    const ImU32 colorB = IM_COL32(150, 150, 150, 255);

    const int cols = static_cast<int>(std::ceil(size.x / cellSize));
    const int rows = static_cast<int>(std::ceil(size.y / cellSize));

    for (int y = 0; y < rows; ++y)
    {
        for (int x = 0; x < cols; ++x)
        {
            const float x0 = minPos.x + x * cellSize;
            const float y0 = minPos.y + y * cellSize;
            const float x1 = std::min(x0 + cellSize, minPos.x + size.x);
            const float y1 = std::min(y0 + cellSize, minPos.y + size.y);

            const bool even = ((x + y) % 2) == 0;
            drawList->AddRectFilled(
                ImVec2(x0, y0),
                ImVec2(x1, y1),
                even ? colorA : colorB);
        }
    }
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

    ImGui::Checkbox("Checker", &_showCheckerboard);

    ImGui::SameLine();

    if (ImGui::SliderFloat("Zoom", &_zoom, 0.1f, 8.0f, "%.2fx"))
    {
        _fitToWindow = false;
    }

    ImGui::SameLine();
    ImGui::Text(" %d%%", static_cast<int>(_zoom * 100.0f));

    ImGui::Separator();

    DrawTextureCanvas();

    ImGui::End();
}
