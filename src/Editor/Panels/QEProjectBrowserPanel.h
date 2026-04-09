#pragma once

#include "IEditorPanel.h"
#include "Editor/Browser/QEProjectAssetItem.h"
#include <string>
#include <memory>
#include <array>
#include <filesystem>
#include <imgui.h>
#include <vulkan/vulkan.h>
#include <unordered_map>
#include <unordered_set>
#include "QEProjectBrowserNavigation.h"
#include "Rendering/QEProjectIconCache.h"

constexpr float tileSize = 92.0f;
constexpr float cellPadding = 12.0f;
constexpr float textPadding = 2.0f;

class QEProjectBrowserPanel : public IEditorPanel
{
public:
    QEProjectBrowserPanel() = default;
    ~QEProjectBrowserPanel() override = default;

    void SetProjectRootPath(const std::filesystem::path& projectRootPath);
    void Draw() override;
    const char* GetName() const override { return "Asset Browser"; }

    void SetPendingExternalDrops(const std::vector<std::filesystem::path>& paths);
    void InitializeIcons();

private:
    void DrawFolderTree(QEProjectAssetItem* item);
    void DrawBreadcrumbAligned(QEProjectAssetItem* folder, float lineHeight);
    void DrawNavigationBar(QEProjectAssetItem* folder);
    void DrawFolderContents(QEProjectAssetItem* folder);
    void DrawAssetTile(QEProjectAssetItem* item, float tileSize);

    std::string GetDisplayAssetName(const QEProjectAssetItem* item) const;
    std::string GetDisplayNameFitted(const std::string& name, float maxWidth) const;

    void HandleExternalFileDrops();
    bool IsImportableExternalFile(const std::filesystem::path& path) const;
    void DrawImportFooter();
    bool HasVisibleImportFooter() const;

    //Folder
    void TryCreateNewFolderInPath(const std::filesystem::path& parentFolderPath);
    void TryDeleteFolderAtPath(const std::filesystem::path& folderPath);
    void ProcessPendingFolderDelete();
    void DrawFolderTreeContextMenu(QEProjectAssetItem* item);
    void DrawAssetTileContextMenu(QEProjectAssetItem* item);
    void DrawContentsBackgroundContextMenu(QEProjectAssetItem* folder);

    //Rename Folder Popup
    void RequestRenamePopup(const std::filesystem::path& targetPath);
    void DrawRenamePopup();

private:
    QEProjectBrowserNavigation _navigation;
    QEProjectIconCache _iconCache;

    std::vector<std::filesystem::path> _pendingExternalDrops;
    bool _isWindowHovered = false;
    bool _isContentsPanelHovered = false;

    //Delete folder
    bool _hasPendingFolderDelete = false;
    std::filesystem::path _pendingFolderDeletePath;

    //Rename folder
    std::filesystem::path _renamePopupTargetPath;
    std::array<char, 256> _renamePopupBuffer{};
    bool _openRenamePopupNextFrame = false;
};
