#pragma once

#include "IEditorPanel.h"
#include "Editor/Browser/QEProjectAssetItem.h"
#include <string>
#include <memory>
#include <filesystem>

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

private:
    std::shared_ptr<QEProjectAssetItem> BuildDirectoryRecursive(
        const std::filesystem::path& path,
        QEProjectAssetItem* parent);

    QEAssetType GetAssetTypeFromPath(const std::filesystem::path& path) const;

    void DrawFolderTree(QEProjectAssetItem* item);
    void DrawFolderContents(QEProjectAssetItem* folder);

    const char* GetAssetTypeLabel(QEAssetType type) const;
    const char* GetAssetIcon(QEAssetType type) const;
};
