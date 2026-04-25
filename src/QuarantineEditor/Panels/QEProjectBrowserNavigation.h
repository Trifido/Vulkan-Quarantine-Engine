#pragma once
#ifndef QE_PROJECT_BROWSER_NAVIGATION
#define QE_PROJECT_BROWSER_NAVIGATION

#include <Browser/QEProjectAssetItem.h>
#include <filesystem>
#include <unordered_set>

class QEProjectBrowserNavigation
{
public:
    void SetProjectRootPath(const std::filesystem::path& projectRootPath);
    void Refresh();

    QEProjectAssetItem* GetRoot() const;
    QEProjectAssetItem* GetSelectedFolder() const;
    QEProjectAssetItem* GetSelectedItem() const;

    QEAssetType GetAssetTypeFromPath(const std::filesystem::path& path) const;

    void SelectFolder(QEProjectAssetItem* folder);
    void SelectItem(QEProjectAssetItem* item);
    void NavigateToFolder(QEProjectAssetItem* folder, bool autoExpandInTree = true);

    bool IsFolderExpanded(const QEProjectAssetItem* item) const;
    void SetFolderExpanded(const QEProjectAssetItem* item, bool expanded);

    bool HasChildDirectories(const QEProjectAssetItem* item) const;
    bool ShouldFlattenFolderInTree(const QEProjectAssetItem* item) const;

    QEProjectAssetItem* FindItemByPath(const std::string& absolutePath);

    QEProjectAssetItem* FindItemByPathRecursive(QEProjectAssetItem* item, const std::string& absolutePath);

    bool ShouldDisplayPath(const std::filesystem::path& path) const;
    bool IsItemDraggable(const QEProjectAssetItem* item) const;

private:
    std::shared_ptr<QEProjectAssetItem> BuildDirectoryRecursive(
        const std::filesystem::path& path,
        QEProjectAssetItem* parent);

    void ExpandDefaultHierarchy();
    void RestoreSelectionPointers();
    QEProjectAssetItem* FindItemByAbsolutePath(
        QEProjectAssetItem* root,
        const std::string& absolutePath) const;

private:
    std::filesystem::path _projectRootPath;
    std::shared_ptr<QEProjectAssetItem> _rootItem = nullptr;

    QEProjectAssetItem* _selectedFolder = nullptr;
    QEProjectAssetItem* _selectedItem = nullptr;

    std::string _selectedFolderPath;
    std::string _selectedItemPath;
    std::unordered_set<std::string> _expandedFolderPaths;
};

#endif // !QE_PROJECT_BROWSER_NAVIGATION
