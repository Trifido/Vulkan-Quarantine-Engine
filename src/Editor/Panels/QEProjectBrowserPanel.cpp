#include "QEProjectBrowserPanel.h"

#include <algorithm>

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

std::string QEProjectBrowserPanel::GetDisplayNameFitted(const std::string& name, float maxWidth) const
{
    if (ImGui::CalcTextSize(name.c_str()).x <= maxWidth)
        return name;

    std::string result = name;
    const std::string ellipsis = "...";

    while (!result.empty())
    {
        result.pop_back();
        std::string candidate = result + ellipsis;
        if (ImGui::CalcTextSize(candidate.c_str()).x <= maxWidth)
            return candidate;
    }

    return ellipsis;
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
        case QEAssetType::Folder:    return "DIR";
        case QEAssetType::Scene:     return "SCN";
        case QEAssetType::Material:  return "MAT";
        case QEAssetType::Texture:   return "TEX";
        case QEAssetType::Mesh:      return "MSH";
        case QEAssetType::Animation: return "ANM";
        default:                     return "UNK";
    }
}

ImVec4 QEProjectBrowserPanel::GetAssetColor(QEAssetType type) const
{
    switch (type)
    {
        case QEAssetType::Folder:    return ImVec4(0.90f, 0.75f, 0.20f, 1.0f);
        case QEAssetType::Scene:     return ImVec4(0.40f, 0.80f, 1.00f, 1.0f);
        case QEAssetType::Material:  return ImVec4(0.80f, 0.40f, 1.00f, 1.0f);
        case QEAssetType::Texture:   return ImVec4(0.30f, 0.90f, 0.30f, 1.0f);
        case QEAssetType::Mesh:      return ImVec4(1.00f, 0.55f, 0.25f, 1.0f);
        case QEAssetType::Animation: return ImVec4(1.00f, 0.35f, 0.35f, 1.0f);
        default:                     return ImVec4(0.70f, 0.70f, 0.70f, 1.0f);
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

    ImGui::Text("Folder: %s", folder->Name.c_str());
    ImGui::Separator();

    float availableWidth = ImGui::GetContentRegionAvail().x;
    int columnCount = static_cast<int>(availableWidth / (tileSize + cellPadding));

    if (columnCount < 1)
        columnCount = 1;

    if (ImGui::BeginTable("ProjectBrowserGrid", columnCount, ImGuiTableFlags_SizingFixedFit))
    {
        int index = 0;

        for (const auto& child : folder->Children)
        {
            ImGui::TableNextColumn();
            DrawAssetTile(child.get(), tileSize);
            ++index;
        }

        ImGui::EndTable();
    }
}

void QEProjectBrowserPanel::DrawAssetTile(QEProjectAssetItem* item, float tileSize)
{
    if (item == nullptr)
        return;

    ImGui::PushID(item->AbsolutePath.c_str());

    const bool isSelected = (_selectedItem == item);

    ImVec2 cursorPos = ImGui::GetCursorScreenPos();
    ImVec2 tileMin = cursorPos;
    ImVec2 iconAreaMax = ImVec2(cursorPos.x + iconAreaSize, cursorPos.y + iconAreaSize);
    ImVec2 tileMax = ImVec2(cursorPos.x + iconAreaSize, cursorPos.y + totalHeight);

    ImGui::InvisibleButton("##tile", ImVec2(iconAreaSize, totalHeight));

    const bool hovered = ImGui::IsItemHovered();
    const bool clicked = ImGui::IsItemClicked();

    if (clicked)
    {
        _selectedItem = item;
    }

    if (hovered && ImGui::IsMouseDoubleClicked(0))
    {
        if (item->IsDirectory)
        {
            _selectedFolder = item;
            _selectedItem = item;
        }
    }

    ImDrawList* drawList = ImGui::GetWindowDrawList();

    ImU32 bgColor = IM_COL32(0, 0, 0, 0);

    if (isSelected)
        bgColor = IM_COL32(70, 110, 170, 160);
    else if (hovered)
        bgColor = IM_COL32(90, 90, 90, 90);

    ImU32 borderColor = hovered || isSelected
        ? IM_COL32(120, 170, 255, 220)
        : IM_COL32(100, 100, 100, 120);

    drawList->AddRectFilled(tileMin, iconAreaMax, bgColor, 6.0f);
    drawList->AddRect(tileMin, iconAreaMax, borderColor, 6.0f);

    // Icono centrado
    const char* icon = GetAssetIcon(item->Type);
    ImVec4 iconColor = GetAssetColor(item->Type);

    ImVec2 iconTextSize = ImGui::CalcTextSize(icon);
    float iconX = tileMin.x + (iconAreaSize - iconTextSize.x) * 0.5f;
    float iconY = tileMin.y + (iconAreaSize - iconTextSize.y) * 0.5f;

    drawList->AddText(
        ImVec2(iconX, iconY),
        ImGui::ColorConvertFloat4ToU32(iconColor),
        icon);

    // Nombre debajo
    const float textPadding = 8.0f;
    std::string label = GetDisplayNameFitted(item->Name, iconAreaSize - textPadding * 2.0f);

    ImVec2 textSize = ImGui::CalcTextSize(label.c_str());
    float textX = tileMin.x + (iconAreaSize - textSize.x) * 0.5f;
    float textY = iconAreaMax.y + 8.0f;

    drawList->AddText(
        ImVec2(textX, textY),
        IM_COL32(230, 230, 230, 255),
        label.c_str());

    ImGui::PopID();
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
