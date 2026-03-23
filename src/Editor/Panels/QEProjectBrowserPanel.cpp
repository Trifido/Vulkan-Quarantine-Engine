#include "QEProjectBrowserPanel.h"

#include <algorithm>
#include <imgui.h>

void QEProjectBrowserPanel::SetProjectRootPath(const std::filesystem::path& projectRootPath)
{
    _projectRootPath = projectRootPath;
    Refresh();
}

QEAssetType QEProjectBrowserPanel::GetAssetTypeFromPath(const std::filesystem::path& path) const
{
    if (std::filesystem::is_directory(path))
        return QEAssetType::Folder;

    std::string ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    if (ext == ".qescene")
        return QEAssetType::Scene;

    if (ext == ".qemat")
        return QEAssetType::Material;

    if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".ktx2")
        return QEAssetType::Texture;

    if (ext == ".gltf" || ext == ".bin")
        return QEAssetType::Mesh;

    if (ext == ".glb")
        return QEAssetType::Animation;

    return QEAssetType::Unknown;
}

const char* QEProjectBrowserPanel::GetAssetTypeLabel(QEAssetType type) const
{
    switch (type)
    {
    case QEAssetType::Folder:     return "Folder";
    case QEAssetType::Scene:      return "Scene";
    case QEAssetType::Material:   return "Material";
    case QEAssetType::Texture:    return "Texture";
    case QEAssetType::Mesh:       return "Mesh";
    case QEAssetType::Animation:  return "Animation";
    default:                      return "Unknown";
    }
}

const char* QEProjectBrowserPanel::GetAssetIcon(QEAssetType type) const
{
    switch (type)
    {
        case QEAssetType::Folder:    return "[DIR]";
        case QEAssetType::Scene:     return "[SCN]";
        case QEAssetType::Material:  return "[MAT]";
        case QEAssetType::Texture:   return "[TEX]";
        case QEAssetType::Mesh:      return "[MSH]";
        case QEAssetType::Animation: return "[ANM]";
        default:                     return "[UNK]";
    }
}

std::shared_ptr<QEProjectAssetItem> QEProjectBrowserPanel::BuildDirectoryRecursive(
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
            entries.push_back(entry);
        }

        std::sort(entries.begin(), entries.end(),
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

void QEProjectBrowserPanel::Refresh()
{
    _rootItem = nullptr;
    _selectedFolder = nullptr;
    _selectedItem = nullptr;

    if (_projectRootPath.empty())
        return;

    if (!std::filesystem::exists(_projectRootPath))
        return;

    _rootItem = BuildDirectoryRecursive(_projectRootPath, nullptr);

    if (_rootItem != nullptr)
    {
        _selectedFolder = _rootItem.get();
    }
}

void QEProjectBrowserPanel::DrawFolderTree(QEProjectAssetItem* item)
{
    if (item == nullptr || !item->IsDirectory)
        return;

    bool hasChildFolders = false;
    for (const auto& child : item->Children)
    {
        if (child->IsDirectory)
        {
            hasChildFolders = true;
            break;
        }
    }

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;

    if (!hasChildFolders)
        flags |= ImGuiTreeNodeFlags_Leaf;

    if (_selectedFolder == item)
        flags |= ImGuiTreeNodeFlags_Selected;

    const bool isOpen = ImGui::TreeNodeEx(
        item->AbsolutePath.c_str(),
        flags,
        "%s %s",
        GetAssetIcon(item->Type),
        item->Name.c_str());

    if (ImGui::IsItemClicked())
    {
        _selectedFolder = item;
        _selectedItem = item;
    }

    if (isOpen)
    {
        for (const auto& child : item->Children)
        {
            if (child->IsDirectory)
                DrawFolderTree(child.get());
        }

        ImGui::TreePop();
    }
}

void QEProjectBrowserPanel::DrawFolderContents(QEProjectAssetItem* folder)
{
    if (folder == nullptr || !folder->IsDirectory)
        return;

    ImGui::Text("Path: %s", folder->RelativePath.c_str());
    ImGui::Separator();

    if (ImGui::BeginTable("ProjectBrowserContents", 4, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_Resizable))
    {
        ImGui::TableSetupColumn("Icon", ImGuiTableColumnFlags_WidthFixed, 60.0f);
        ImGui::TableSetupColumn("Name");
        ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableSetupColumn("Relative Path");
        ImGui::TableHeadersRow();

        for (const auto& child : folder->Children)
        {
            QEProjectAssetItem* childPtr = child.get();

            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            ImGui::TextUnformatted(GetAssetIcon(childPtr->Type));

            ImGui::TableSetColumnIndex(1);
            const bool isSelected = (_selectedItem == childPtr);
            if (ImGui::Selectable(childPtr->Name.c_str(), isSelected, ImGuiSelectableFlags_SpanAllColumns))
            {
                _selectedItem = childPtr;
            }

            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
            {
                if (childPtr->IsDirectory)
                {
                    _selectedFolder = childPtr;
                    _selectedItem = childPtr;
                }
            }

            ImGui::TableSetColumnIndex(2);
            ImGui::TextUnformatted(GetAssetTypeLabel(childPtr->Type));

            ImGui::TableSetColumnIndex(3);
            ImGui::TextUnformatted(childPtr->RelativePath.c_str());
        }

        ImGui::EndTable();
    }
}

void QEProjectBrowserPanel::Draw()
{
    if (!ImGui::Begin("Project Browser"))
    {
        ImGui::End();
        return;
    }

    if (ImGui::Button("Refresh"))
    {
        Refresh();
    }

    ImGui::Separator();

    if (_rootItem == nullptr)
    {
        ImGui::TextUnformatted("No project loaded.");
        ImGui::End();
        return;
    }

    ImGui::Columns(2, "ProjectBrowserColumns", true);

    ImGui::TextUnformatted("Folders");
    ImGui::Separator();
    DrawFolderTree(_rootItem.get());

    ImGui::NextColumn();

    ImGui::TextUnformatted("Contents");
    ImGui::Separator();
    DrawFolderContents(_selectedFolder);

    ImGui::Columns(1);

    ImGui::End();
}
