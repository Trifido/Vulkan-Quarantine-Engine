#pragma once

#include "Editor/Browser/QEProjectAssetItem.h"
#include "Editor/Panels/QEProjectBrowserNavigation.h"
#include <array>
#include <filesystem>

class QEProjectBrowserActions
{
public:
    explicit QEProjectBrowserActions(QEProjectBrowserNavigation& navigation);

    void DrawFolderTreeContextMenu(QEProjectAssetItem* item);
    void DrawAssetTileContextMenu(QEProjectAssetItem* item);
    void DrawContentsBackgroundContextMenu(QEProjectAssetItem* folder);

    void DrawRenamePopup();
    void ProcessPendingDelete();

private:
    void TryCreateNewFolderInPath(const std::filesystem::path& parentFolderPath);

    void RequestRename(const std::filesystem::path& targetPath);
    bool ExecuteRename();

    void RequestDelete(const std::filesystem::path& targetPath);
    const char* GetDeleteMenuLabel(const QEProjectAssetItem* item) const;
    const char* GetRenamePopupTitle(const QEProjectAssetItem* item) const;
    const char* GetRenamePopupLabel(const QEProjectAssetItem* item) const;

private:
    QEProjectBrowserNavigation& _navigation;

    bool _hasPendingDelete = false;
    std::filesystem::path _pendingDeletePath;

    std::filesystem::path _renamePopupTargetPath;
    QEProjectAssetItem* _renamePopupTargetItem = nullptr;
    std::array<char, 256> _renamePopupBuffer{};
    bool _openRenamePopupNextFrame = false;
};
