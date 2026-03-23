#pragma once

#include "IEditorPanel.h"
#include "Editor/Browser/QEProjectAssetItem.h"
#include <string>
#include <memory>
#include <filesystem>
#include <imgui.h>

constexpr float tileSize = 80.0f;
constexpr float cellPadding = 12.0f;

class QEProjectBrowserPanel : public IEditorPanel
{
public:
    QEProjectBrowserPanel() = default;
    ~QEProjectBrowserPanel() override = default;

    void SetProjectRootPath(const std::filesystem::path& projectRootPath);
    const std::filesystem::path& GetProjectRootPath() const { return _projectRootPath; }
    void Refresh();
    void Draw() override;

    const char* GetName() const override { return "Asset Browser"; }

private:
    std::filesystem::path _projectRootPath;
    std::shared_ptr<QEProjectAssetItem> _rootItem = nullptr;

    QEProjectAssetItem* _selectedFolder = nullptr;
    QEProjectAssetItem* _selectedItem = nullptr;

    const float iconAreaSize = tileSize;
    const float labelHeight = 32.0f;
    const float totalHeight = iconAreaSize + labelHeight;

private:
    std::shared_ptr<QEProjectAssetItem> BuildDirectoryRecursive(
        const std::filesystem::path& path,
        QEProjectAssetItem* parent);

    QEAssetType GetAssetTypeFromPath(const std::filesystem::path& path) const;
    std::string GetDisplayNameFitted(const std::string& name, float maxWidth) const;

    void DrawFolderTree(QEProjectAssetItem* item);
    void DrawFolderContents(QEProjectAssetItem* folder);
    void DrawAssetTile(QEProjectAssetItem* item, float tileSize);

    const char* GetAssetTypeLabel(QEAssetType type) const;
    const char* GetAssetIcon(QEAssetType type) const;
    ImVec4 GetAssetColor(QEAssetType type) const;
};
