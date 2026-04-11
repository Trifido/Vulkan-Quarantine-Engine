#include "QEProjectBrowserPanel.h"

#include <algorithm>
#include <imgui.h>

#include <QEProjectManager.h>
#include <QEAssetImportManager.h>
#include <iostream>
#include <cstring>
#include <Logging/QELogMacros.h>
#include <Browser/QEProjectAssetCreator.h>

namespace
{
    bool IsVisibleImportJobState(QEImportJobState state)
    {
        return state == QEImportJobState::Queued ||
            state == QEImportJobState::Running ||
            state == QEImportJobState::Failed;
    }
}

QEProjectBrowserPanel::QEProjectBrowserPanel()
    : _actions(_navigation)
{
}

void QEProjectBrowserPanel::SetProjectRootPath(const std::filesystem::path& projectRootPath)
{
    _navigation.SetProjectRootPath(projectRootPath);
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

std::string QEProjectBrowserPanel::GetDisplayAssetName(const QEProjectAssetItem* item) const
{
    if (item == nullptr)
        return "";

    const std::string name = item->Name;
    if (name.empty())
        return "";

    if (item->IsDirectory)
        return name;

    std::filesystem::path p(name);
    return p.stem().string();
}

void QEProjectBrowserPanel::DrawFolderTree(QEProjectAssetItem* item)
{
    if (item == nullptr || !item->IsDirectory)
        return;

    const bool hasChildFolders = _navigation.HasChildDirectories(item);
    const bool flattenInTree = _navigation.ShouldFlattenFolderInTree(item);

    ImGuiTreeNodeFlags flags =
        ImGuiTreeNodeFlags_OpenOnArrow |
        ImGuiTreeNodeFlags_OpenOnDoubleClick;

    if (!hasChildFolders || flattenInTree)
        flags |= ImGuiTreeNodeFlags_Leaf;

    if (_navigation.GetSelectedFolder() == item)
        flags |= ImGuiTreeNodeFlags_Selected;

    const bool shouldBeOpen = _navigation.IsFolderExpanded(item);
    ImGui::SetNextItemOpen(shouldBeOpen, ImGuiCond_Always);

    const bool isOpen = ImGui::TreeNodeEx(
        item->AbsolutePath.c_str(),
        flags,
        "%s",
        item->Name.c_str());

    if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
    {
        _navigation.SelectFolder(item);
    }

    if (ImGui::IsItemToggledOpen())
    {
        _navigation.SetFolderExpanded(item, isOpen);
    }

    _actions.DrawFolderTreeContextMenu(item);

    if (isOpen)
    {
        if (!(flags & ImGuiTreeNodeFlags_Leaf))
        {
            for (const auto& child : item->Children)
            {
                if (child && child->IsDirectory)
                    DrawFolderTree(child.get());
            }
        }

        ImGui::TreePop();
    }
}

void QEProjectBrowserPanel::DrawBreadcrumbAligned(QEProjectAssetItem* folder, float lineHeight)
{
    std::vector<QEProjectAssetItem*> path;

    QEProjectAssetItem* current = folder;
    while (current)
    {
        path.push_back(current);
        current = current->Parent;
    }

    std::reverse(path.begin(), path.end());

    for (size_t i = 0; i < path.size(); ++i)
    {
        QEProjectAssetItem* node = path[i];

        // 🔑 mismo baseline SIEMPRE
        ImGui::AlignTextToFramePadding();

        // 🔑 usar Button en vez de SmallButton (más consistente)
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6, 2));

        if (ImGui::Button(node->Name.c_str()))
        {
            _navigation.NavigateToFolder(node, true);
        }

        ImGui::PopStyleVar();

        if (i < path.size() - 1)
        {
            ImGui::SameLine(0, 4.0f);

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted(">");

            ImGui::SameLine(0, 4.0f);
        }
    }
}

void QEProjectBrowserPanel::DrawNavigationBar(QEProjectAssetItem* folder)
{
    const float lineHeight = ImGui::GetTextLineHeight();
    const float iconSize = 17.f;

    // 🔑 Esto fuerza baseline común para TODO
    ImGui::AlignTextToFramePadding();

    // =========================
    // ⬆ BOTÓN UP
    // =========================
    const QEIconTexture* upIcon = _iconCache.GetIcon(QEAssetType::NavigateUp);

    if (upIcon && upIcon->ImGuiTexture != 0)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(255, 255, 255, 25));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(255, 255, 255, 40));

        // 🔑 IMPORTANTE: padding 0
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));

        if (ImGui::ImageButton("##Up", upIcon->ImGuiTexture, ImVec2(iconSize, iconSize)))
        {
            if (folder->Parent)
                _navigation.NavigateToFolder(folder->Parent, true);
        }

        ImGui::PopStyleVar();
        ImGui::PopStyleColor(3);
    }

    ImGui::SameLine(0, 6.0f);

    // =========================
    // 🧭 BREADCRUMB
    // =========================
    DrawBreadcrumbAligned(folder, lineHeight);
}

void QEProjectBrowserPanel::DrawFolderContents(QEProjectAssetItem* folder)
{
    if (folder == nullptr || !folder->IsDirectory)
        return;

    DrawNavigationBar(folder);

    ImGui::Separator();
    ImGui::Spacing();

    const float availableWidth = ImGui::GetContentRegionAvail().x;
    int columnCount = static_cast<int>(availableWidth / (tileSize + cellPadding));
    if (columnCount < 1)
        columnCount = 1;

    if (ImGui::BeginTable("ProjectBrowserGrid", columnCount, ImGuiTableFlags_SizingFixedFit))
    {
        for (const auto& child : folder->Children)
        {
            if (!child) continue;

            ImGui::TableNextColumn();
            DrawAssetTile(child.get(), tileSize);
        }

        ImGui::EndTable();
    }

    _actions.DrawContentsBackgroundContextMenu(folder);
}

void QEProjectBrowserPanel::DrawAssetTile(QEProjectAssetItem* item, float tileSizeValue)
{
    if (item == nullptr)
        return;

    ImGui::PushID(item->AbsolutePath.c_str());

    const bool isSelected = (_navigation.GetSelectedItem() == item);
    const float iconAreaSize = tileSizeValue;
    const float labelHeight = 28.0f;
    const float totalHeight = iconAreaSize + labelHeight;

    ImVec2 cursorPos = ImGui::GetCursorScreenPos();
    ImVec2 tileMin = cursorPos;
    ImVec2 iconAreaMax = ImVec2(cursorPos.x + iconAreaSize, cursorPos.y + iconAreaSize);

    ImGui::InvisibleButton("##tile", ImVec2(iconAreaSize, totalHeight));

    const bool hovered = ImGui::IsItemHovered();

    if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
    {
        _navigation.SelectItem(item);
    }

    if (hovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
    {
        if (item->IsDirectory)
        {
            _navigation.NavigateToFolder(item, true);
        }
        else
        {
            std::filesystem::path assetPath(item->AbsolutePath);
            std::string ext = assetPath.extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

            if (ext == ".qescene")
            {
                _pendingSceneOpenRequest = assetPath;
            }
            else if (IsTextureAsset(assetPath))
            {
                _pendingTextureOpenRequest = assetPath;
            }
        }
    }

    if (_navigation.IsItemDraggable(item))
    {
        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
        {
            const std::string fullPath = item->AbsolutePath;

            ImGui::SetDragDropPayload(
                "QE_PROJECT_ASSET_PATH",
                fullPath.c_str(),
                (fullPath.size() + 1) * sizeof(char));

            ImGui::TextUnformatted("Mesh");
            ImGui::Separator();
            ImGui::TextUnformatted(item->Name.c_str());

            ImGui::EndDragDropSource();
        }
    }

    ImDrawList* drawList = ImGui::GetWindowDrawList();

    ImU32 bgColor = IM_COL32(0, 0, 0, 0);
    if (isSelected)
        bgColor = IM_COL32(70, 110, 170, 160);
    else if (hovered)
        bgColor = IM_COL32(90, 90, 90, 90);

    ImU32 borderColor = (hovered || isSelected)
        ? IM_COL32(120, 170, 255, 220)
        : IM_COL32(100, 100, 100, 120);

    drawList->AddRectFilled(tileMin, iconAreaMax, bgColor, 6.0f);
    drawList->AddRect(tileMin, iconAreaMax, borderColor, 6.0f);

    const QEIconTexture* iconTexture = _iconCache.GetIcon(item->Type);

    if (iconTexture && iconTexture->ImGuiTexture != 0)
    {
        const float imagePadding = 10.0f;
        ImVec2 imageMin(tileMin.x + imagePadding, tileMin.y + imagePadding);
        ImVec2 imageMax(iconAreaMax.x - imagePadding, iconAreaMax.y - imagePadding);

        drawList->AddImage(
            (ImTextureID)iconTexture->ImGuiTexture,
            imageMin,
            imageMax);
    }
    else
    {
        const char* fallback = "UNK";
        ImVec2 textSize = ImGui::CalcTextSize(fallback);
        float textX = tileMin.x + (iconAreaSize - textSize.x) * 0.5f;
        float textY = tileMin.y + (iconAreaSize - textSize.y) * 0.5f;

        drawList->AddText(
            ImVec2(textX, textY),
            IM_COL32(210, 210, 210, 255),
            fallback);
    }

    std::string baseName = GetDisplayAssetName(item);
    std::string label = GetDisplayNameFitted(baseName, iconAreaSize - textPadding * 2.0f);

    ImVec2 textSize = ImGui::CalcTextSize(label.c_str());
    float textX = tileMin.x + (iconAreaSize - textSize.x) * 0.5f;
    float textY = iconAreaMax.y + 8.0f;

    drawList->AddText(
        ImVec2(textX, textY),
        IM_COL32(210, 210, 210, 255),
        label.c_str());

    _actions.DrawAssetTileContextMenu(item);

    ImGui::PopID();
}

void QEProjectBrowserPanel::Draw()
{
    QEAssetImportManager::Get().UpdateMainThread();
    if (QEAssetImportManager::Get().ConsumeFinishedSuccessfulImports() > 0)
    {
        _navigation.Refresh();
    }

    _isWindowHovered = false;
    _isContentsPanelHovered = false;

    if (!ImGui::Begin("Project Browser"))
    {
        _actions.DrawRenamePopup();
        _actions.ProcessPendingDelete();
        ImGui::End();
        return;
    }

    _isWindowHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows);

    DrawTopBar();

    ImGui::Spacing();

    if (_navigation.GetRoot() == nullptr)
    {
        ImGui::TextUnformatted("No project loaded.");
        HandleExternalFileDrops();
        DrawImportFooter();

        _actions.DrawRenamePopup();
        _actions.ProcessPendingDelete();
        ImGui::End();

        return;
    }

    const float footerHeight = HasVisibleImportFooter() ? 120.0f : 0.0f;

    const ImGuiTableFlags tableFlags =
        ImGuiTableFlags_Resizable |
        ImGuiTableFlags_SizingStretchProp |
        ImGuiTableFlags_BordersInnerV;

    if (ImGui::BeginChild("ProjectBrowserMainArea", ImVec2(0, -footerHeight), false))
    {
        if (ImGui::BeginTable("ProjectBrowserLayout", 2, tableFlags))
        {
            ImGui::TableSetupColumn("Folders", ImGuiTableColumnFlags_WidthStretch, 0.32f);
            ImGui::TableSetupColumn("Contents", ImGuiTableColumnFlags_WidthStretch, 0.68f);

            ImGui::TableNextColumn();

            ImGui::TextUnformatted("Folders");
            ImGui::Spacing();

            if (ImGui::BeginChild("FoldersPanel", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar))
            {
                DrawFolderTree(_navigation.GetRoot());
            }
            ImGui::EndChild();

            ImGui::TableNextColumn();

            ImGui::TextUnformatted("Contents");
            ImGui::Spacing();

            if (ImGui::BeginChild("ContentsPanel", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar))
            {
                _isContentsPanelHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows);
                DrawFolderContents(_navigation.GetSelectedFolder());
            }
            ImGui::EndChild();

            ImGui::EndTable();
        }
    }
    ImGui::EndChild();

    HandleExternalFileDrops();
    DrawImportFooter();

    _actions.DrawRenamePopup();
    _actions.ProcessPendingDelete();
    ImGui::End();
}

void QEProjectBrowserPanel::SetPendingExternalDrops(const std::vector<std::filesystem::path>& paths)
{
    _pendingExternalDrops = paths;
}

void QEProjectBrowserPanel::InitializeIcons()
{
    _iconCache.Initialize();
}

std::optional<std::filesystem::path> QEProjectBrowserPanel::ConsumePendingSceneOpenRequest()
{
    if (!_pendingSceneOpenRequest.has_value())
        return std::nullopt;

    auto result = _pendingSceneOpenRequest;
    _pendingSceneOpenRequest.reset();
    return result;
}

bool QEProjectBrowserPanel::IsImportableExternalFile(const std::filesystem::path& path) const
{
    if (!std::filesystem::exists(path))
        return false;

    if (std::filesystem::is_directory(path))
        return false;

    std::string ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    return ext == ".gltf" || ext == ".glb" || ext == ".fbx" || ext == ".obj";
}

void QEProjectBrowserPanel::DrawImportFooter()
{
    auto jobs = QEAssetImportManager::Get().GetJobsSnapshot();
    if (jobs.empty())
        return;

    bool hasVisibleJobs = false;
    for (const auto& job : jobs)
    {
        if (!job)
            continue;

        if (IsVisibleImportJobState(job->State.load(std::memory_order_relaxed)))
        {
            hasVisibleJobs = true;
            break;
        }
    }

    if (!hasVisibleJobs)
        return;

    ImGui::Separator();
    ImGui::TextUnformatted("Imports");

    for (const auto& job : jobs)
    {
        if (!job)
            continue;

        const QEImportJobState state = job->State.load(std::memory_order_relaxed);
        if (state == QEImportJobState::Succeeded)
            continue;

        const float progress = job->Progress.Value.load(std::memory_order_relaxed);
        const std::string stage = job->Progress.Stage;
        const std::string message = job->Progress.Message;

        std::string title = job->DisplayName;
        if (!stage.empty())
            title += " - " + stage;

        ImGui::TextUnformatted(title.c_str());

        std::string overlay;
        switch (state)
        {
            case QEImportJobState::Queued:  overlay = "Queued"; break;
            case QEImportJobState::Running: overlay = message.empty() ? "Importing..." : message; break;
            case QEImportJobState::Failed:  overlay = "Failed"; break;
            default:                        overlay.clear(); break;
        }

        ImGui::ProgressBar(progress, ImVec2(-1.0f, 0.0f), overlay.c_str());

        if (state == QEImportJobState::Failed && !job->Error.empty())
        {
            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 120, 120, 255));
            ImGui::TextWrapped("%s", job->Error.c_str());
            ImGui::PopStyleColor();
        }

        ImGui::Spacing();
    }
}

bool QEProjectBrowserPanel::HasVisibleImportFooter() const
{
    auto jobs = QEAssetImportManager::Get().GetJobsSnapshot();

    for (const auto& job : jobs)
    {
        if (!job)
            continue;

        if (IsVisibleImportJobState(job->State.load(std::memory_order_relaxed)))
            return true;
    }

    return false;
}

void QEProjectBrowserPanel::HandleExternalFileDrops()
{
    if (_pendingExternalDrops.empty())
        return;

    if (!_isWindowHovered && !_isContentsPanelHovered)
        return;

    QEProjectAssetItem* selectedFolder = _navigation.GetSelectedFolder();
    if (selectedFolder == nullptr || !selectedFolder->IsDirectory)
    {
        _pendingExternalDrops.clear();
        return;
    }

    const std::string targetFolder = selectedFolder->AbsolutePath;

    for (const auto& droppedPath : _pendingExternalDrops)
    {
        if (!IsImportableExternalFile(droppedPath))
            continue;

        try
        {
            QEAssetImportManager::Get().EnqueueMeshImport(
                droppedPath.string(),
                targetFolder);
        }
        catch (const std::exception& e)
        {
            QE_LOG_ERROR_CAT_F("QEProjectBrowserPanel", "Import failed: {}", e.what());
        }
    }

    _pendingExternalDrops.clear();
}

void QEProjectBrowserPanel::DrawTopBar()
{
    if (ImGui::Button("Refresh"))
        _navigation.Refresh();

    ImGui::SameLine();

    DrawCreateMenu(_navigation.GetSelectedFolder());
}

void QEProjectBrowserPanel::DrawCreateMenu(QEProjectAssetItem* currentFolder)
{
    if (currentFolder == nullptr || !currentFolder->IsDirectory)
        return;

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6.0f, 3.0f));

    if (ImGui::Button("Create"))
    {
        ImGui::OpenPopup("CreateAssetPopup");
    }

    ImGui::PopStyleVar();

    if (ImGui::BeginPopup("CreateAssetPopup"))
    {
        if (ImGui::MenuItem("Scene"))
        {
            if (QEProjectAssetCreator::CreateSceneAt(currentFolder->AbsolutePath, "New Scene"))
            {
                _navigation.Refresh();
            }
        }

        if (ImGui::MenuItem("Material"))
        {
            if (QEProjectAssetCreator::CreateMaterialAt(currentFolder->AbsolutePath, "New Material"))
            {
                _navigation.Refresh();
            }
        }

        ImGui::EndPopup();
    }
}

bool QEProjectBrowserPanel::IsTextureAsset(const std::filesystem::path& path) const
{
    std::string ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    return ext == ".png" ||
        ext == ".jpg" ||
        ext == ".jpeg" ||
        ext == ".ktx2" ||
        ext == ".dds" ||
        ext == ".tga" ||
        ext == ".bmp";
}

std::optional<std::filesystem::path> QEProjectBrowserPanel::ConsumePendingTextureOpenRequest()
{
    if (!_pendingTextureOpenRequest.has_value())
        return std::nullopt;

    auto result = _pendingTextureOpenRequest;
    _pendingTextureOpenRequest.reset();
    return result;
}
