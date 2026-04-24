#include "QEProjectBrowserNavigation.h"

#include <algorithm>

void QEProjectBrowserNavigation::SetProjectRootPath(const std::filesystem::path& projectRootPath)
{
    _projectRootPath = projectRootPath;
    Refresh();
}

QEProjectAssetItem* QEProjectBrowserNavigation::GetRoot() const
{
    return _rootItem.get();
}

QEProjectAssetItem* QEProjectBrowserNavigation::GetSelectedFolder() const
{
    return _selectedFolder;
}

QEProjectAssetItem* QEProjectBrowserNavigation::GetSelectedItem() const
{
    return _selectedItem;
}

QEAssetType QEProjectBrowserNavigation::GetAssetTypeFromPath(const std::filesystem::path& path) const
{
    if (std::filesystem::is_directory(path))
        return QEAssetType::Folder;

    std::string ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    if (ext == ".qescene") return QEAssetType::Scene;
    if (ext == ".qemat")   return QEAssetType::Material;
    if (ext == ".qeshader") return QEAssetType::Shader;
    if (ext == ".spv")     return QEAssetType::Shader;
    if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".ktx2") return QEAssetType::Texture;
    if (ext == ".gltf")    return QEAssetType::Mesh;
    if (ext == ".glb")     return QEAssetType::Animation;
    if (ext == ".bin")     return QEAssetType::Unknown;

    return QEAssetType::Unknown;
}

bool QEProjectBrowserNavigation::ShouldDisplayPath(const std::filesystem::path& path) const
{
    if (std::filesystem::is_directory(path))
        return true;

    std::string ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    return ext != ".bin";
}

bool QEProjectBrowserNavigation::IsItemDraggable(const QEProjectAssetItem* item) const
{
    if (item == nullptr || item->IsDirectory)
        return false;

    std::filesystem::path p(item->AbsolutePath);
    std::string ext = p.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    return ext == ".gltf";
}

std::shared_ptr<QEProjectAssetItem> QEProjectBrowserNavigation::BuildDirectoryRecursive(
    const std::filesystem::path& path,
    QEProjectAssetItem* parent)
{
    auto item = std::make_shared<QEProjectAssetItem>();

    item->Name = path.filename().string();
    if (item->Name.empty())
        item->Name = path.string();

    item->AbsolutePath = path.string();

    try
    {
        item->RelativePath = std::filesystem::relative(path, _projectRootPath).string();
    }
    catch (...)
    {
        item->RelativePath = item->AbsolutePath;
    }

    item->IsDirectory = std::filesystem::is_directory(path);
    item->Type = GetAssetTypeFromPath(path);
    item->Parent = parent;

    if (item->IsDirectory)
    {
        std::vector<std::filesystem::directory_entry> entries;

        for (const auto& entry : std::filesystem::directory_iterator(path))
        {
            if (!ShouldDisplayPath(entry.path()))
                continue;

            entries.push_back(entry);
        }

        std::sort(
            entries.begin(),
            entries.end(),
            [](const auto& a, const auto& b)
            {
                const bool aIsDir = a.is_directory();
                const bool bIsDir = b.is_directory();

                if (aIsDir != bIsDir)
                    return aIsDir > bIsDir;

                return a.path().filename().string() < b.path().filename().string();
            });

        for (const auto& entry : entries)
        {
            item->Children.push_back(BuildDirectoryRecursive(entry.path(), item.get()));
        }
    }

    return item;
}

void QEProjectBrowserNavigation::Refresh()
{
    const std::string previousSelectedFolderPath = _selectedFolderPath;
    const std::string previousSelectedItemPath = _selectedItemPath;
    const bool hadExpandedState = !_expandedFolderPaths.empty();

    _rootItem = nullptr;
    _selectedFolder = nullptr;
    _selectedItem = nullptr;

    if (_projectRootPath.empty())
        return;

    if (!std::filesystem::exists(_projectRootPath))
        return;

    _rootItem = BuildDirectoryRecursive(_projectRootPath, nullptr);
    if (_rootItem == nullptr)
        return;

    if (!hadExpandedState)
        ExpandDefaultHierarchy();

    _selectedFolderPath = previousSelectedFolderPath;
    _selectedItemPath = previousSelectedItemPath;

    RestoreSelectionPointers();

    if (_selectedFolder == nullptr)
    {
        _selectedFolder = _rootItem.get();
        _selectedFolderPath = _selectedFolder->AbsolutePath;
    }

    if (_selectedItem == nullptr && !_selectedItemPath.empty())
    {
        _selectedItemPath.clear();
    }
}

void QEProjectBrowserNavigation::ExpandDefaultHierarchy()
{
    if (_rootItem == nullptr)
        return;

    _expandedFolderPaths.clear();
    _expandedFolderPaths.insert(_rootItem->AbsolutePath);

    for (const auto& child : _rootItem->Children)
    {
        if (child && child->IsDirectory)
            _expandedFolderPaths.insert(child->AbsolutePath);
    }
}

void QEProjectBrowserNavigation::RestoreSelectionPointers()
{
    if (_rootItem == nullptr)
        return;

    if (!_selectedFolderPath.empty())
        _selectedFolder = FindItemByAbsolutePath(_rootItem.get(), _selectedFolderPath);

    if (!_selectedItemPath.empty())
        _selectedItem = FindItemByAbsolutePath(_rootItem.get(), _selectedItemPath);
}

QEProjectAssetItem* QEProjectBrowserNavigation::FindItemByAbsolutePath(
    QEProjectAssetItem* root,
    const std::string& absolutePath) const
{
    if (root == nullptr)
        return nullptr;

    if (root->AbsolutePath == absolutePath)
        return root;

    for (const auto& child : root->Children)
    {
        if (!child)
            continue;

        if (QEProjectAssetItem* found = FindItemByAbsolutePath(child.get(), absolutePath))
            return found;
    }

    return nullptr;
}

void QEProjectBrowserNavigation::SelectFolder(QEProjectAssetItem* folder)
{
    if (folder == nullptr || !folder->IsDirectory)
        return;

    _selectedFolder = folder;
    _selectedFolderPath = folder->AbsolutePath;

    _selectedItem = nullptr;
    _selectedItemPath.clear();
}

void QEProjectBrowserNavigation::SelectItem(QEProjectAssetItem* item)
{
    if (item == nullptr)
        return;

    _selectedItem = item;
    _selectedItemPath = item->AbsolutePath;

    if (item->IsDirectory)
    {
        _selectedFolder = item;
        _selectedFolderPath = item->AbsolutePath;
    }
}

void QEProjectBrowserNavigation::NavigateToFolder(QEProjectAssetItem* folder, bool autoExpandInTree)
{
    if (folder == nullptr || !folder->IsDirectory)
        return;

    _selectedFolder = folder;
    _selectedFolderPath = folder->AbsolutePath;

    _selectedItem = nullptr;
    _selectedItemPath.clear();

    if (autoExpandInTree)
    {
        QEProjectAssetItem* current = folder;
        while (current != nullptr)
        {
            _expandedFolderPaths.insert(current->AbsolutePath);
            current = current->Parent;
        }
    }
}

bool QEProjectBrowserNavigation::IsFolderExpanded(const QEProjectAssetItem* item) const
{
    if (item == nullptr)
        return false;

    return _expandedFolderPaths.find(item->AbsolutePath) != _expandedFolderPaths.end();
}

void QEProjectBrowserNavigation::SetFolderExpanded(const QEProjectAssetItem* item, bool expanded)
{
    if (item == nullptr)
        return;

    if (expanded)
        _expandedFolderPaths.insert(item->AbsolutePath);
    else
        _expandedFolderPaths.erase(item->AbsolutePath);
}

bool QEProjectBrowserNavigation::HasChildDirectories(const QEProjectAssetItem* item) const
{
    if (item == nullptr || !item->IsDirectory)
        return false;

    for (const auto& child : item->Children)
    {
        if (child && child->IsDirectory)
            return true;
    }

    return false;
}

bool QEProjectBrowserNavigation::ShouldFlattenFolderInTree(const QEProjectAssetItem* item) const
{
    if (item == nullptr || !item->IsDirectory)
        return false;

    if (!HasChildDirectories(item))
        return true;

    bool onlyKnownAssetSubfolders = true;

    for (const auto& child : item->Children)
    {
        if (!child || !child->IsDirectory)
            continue;

        const std::string& n = child->Name;
        if (n != "Animations" &&
            n != "Materials" &&
            n != "Meshes" &&
            n != "Textures" &&
            n != "source")
        {
            onlyKnownAssetSubfolders = false;
            break;
        }
    }

    return onlyKnownAssetSubfolders;
}

QEProjectAssetItem* QEProjectBrowserNavigation::FindItemByPath(const std::string& absolutePath)
{
    return FindItemByPathRecursive(_rootItem.get(), absolutePath);
}

QEProjectAssetItem* QEProjectBrowserNavigation::FindItemByPathRecursive(QEProjectAssetItem* item, const std::string& absolutePath)
{
    if (item == nullptr)
        return nullptr;

    if (item->AbsolutePath == absolutePath)
        return item;

    for (const auto& child : item->Children)
    {
        if (QEProjectAssetItem* found = FindItemByPathRecursive(child.get(), absolutePath))
            return found;
    }

    return nullptr;
}
