#include "QEProjectBrowserActions.h"

#include <QEProjectManager.h>
#include <imgui.h>
#include <cstring>

QEProjectBrowserActions::QEProjectBrowserActions(QEProjectBrowserNavigation& navigation)
    : _navigation(navigation)
{
    _renamePopupBuffer.fill('\0');
}

void QEProjectBrowserActions::TryCreateNewFolderInPath(const std::filesystem::path& parentFolderPath)
{
    if (QEProjectManager::CreateUniqueFolderAt(parentFolderPath, "New Folder"))
    {
        _navigation.Refresh();
    }
}

void QEProjectBrowserActions::RequestDelete(const std::filesystem::path& targetPath)
{
    _hasPendingDelete = true;
    _pendingDeletePath = targetPath;
}

void QEProjectBrowserActions::ProcessPendingDelete()
{
    if (!_hasPendingDelete)
        return;

    const std::filesystem::path targetPath = _pendingDeletePath;

    _hasPendingDelete = false;
    _pendingDeletePath.clear();

    if (QEProjectManager::DeletePath(targetPath, true))
    {
        _navigation.Refresh();
    }
}

void QEProjectBrowserActions::RequestRename(const std::filesystem::path& targetPath)
{
    _renamePopupTargetPath = targetPath;
    _renamePopupTargetItem = _navigation.FindItemByPath(targetPath.string());
    _renamePopupBuffer.fill('\0');

    const std::string currentName = targetPath.filename().string();
    std::strncpy(_renamePopupBuffer.data(), currentName.c_str(), _renamePopupBuffer.size() - 1);
    _renamePopupBuffer[_renamePopupBuffer.size() - 1] = '\0';

    _openRenamePopupNextFrame = true;
}

bool QEProjectBrowserActions::ExecuteRename()
{
    std::string newName = _renamePopupBuffer.data();
    if (newName.empty())
        return false;

    const std::filesystem::path sourcePath(_renamePopupTargetPath);

    if (_renamePopupTargetItem != nullptr && !_renamePopupTargetItem->IsDirectory)
    {
        const std::string originalExtension = sourcePath.extension().string();
        const std::filesystem::path typedPath(newName);

        if (typedPath.extension().empty() && !originalExtension.empty())
        {
            newName += originalExtension;
        }
    }

    if (!QEProjectManager::RenamePath(_renamePopupTargetPath, newName))
        return false;

    _navigation.Refresh();
    _renamePopupTargetPath.clear();
    _renamePopupTargetItem = nullptr;
    _renamePopupBuffer.fill('\0');
    return true;
}

const char* QEProjectBrowserActions::GetDeleteMenuLabel(const QEProjectAssetItem* item) const
{
    if (item == nullptr)
        return "Delete";

    switch (item->Type)
    {
    case QEAssetType::Folder:    return "Delete Folder";
    case QEAssetType::Scene:     return "Delete Scene";
    case QEAssetType::Material:  return "Delete Material";
    case QEAssetType::Shader:    return "Delete Shader";
    case QEAssetType::Texture:   return "Delete Texture";
    case QEAssetType::Mesh:      return "Delete Mesh";
    case QEAssetType::Animation: return "Delete Animation";
    case QEAssetType::Unknown:   return "Delete Asset";
    default:                     return "Delete";
    }
}

const char* QEProjectBrowserActions::GetRenamePopupTitle(const QEProjectAssetItem* item) const
{
    if (item == nullptr)
        return "Rename Asset";

    switch (item->Type)
    {
    case QEAssetType::Folder:    return "Rename Folder";
    case QEAssetType::Scene:     return "Rename Scene";
    case QEAssetType::Material:  return "Rename Material";
    case QEAssetType::Shader:    return "Rename Shader";
    case QEAssetType::Texture:   return "Rename Texture";
    case QEAssetType::Mesh:      return "Rename Mesh";
    case QEAssetType::Animation: return "Rename Animation";
    case QEAssetType::Unknown:   return "Rename Asset";
    default:                     return "Rename";
    }
}

const char* QEProjectBrowserActions::GetRenamePopupLabel(const QEProjectAssetItem* item) const
{
    if (item == nullptr)
        return "Rename asset";

    switch (item->Type)
    {
    case QEAssetType::Folder:    return "Rename folder";
    case QEAssetType::Scene:     return "Rename scene";
    case QEAssetType::Material:  return "Rename material";
    case QEAssetType::Shader:    return "Rename shader";
    case QEAssetType::Texture:   return "Rename texture";
    case QEAssetType::Mesh:      return "Rename mesh";
    case QEAssetType::Animation: return "Rename animation";
    case QEAssetType::Unknown:   return "Rename asset";
    default:                     return "Rename";
    }
}

void QEProjectBrowserActions::DrawFolderTreeContextMenu(QEProjectAssetItem* item)
{
    if (item == nullptr || !item->IsDirectory)
        return;

    if (ImGui::BeginPopupContextItem())
    {
        if (ImGui::MenuItem("New Folder"))
        {
            TryCreateNewFolderInPath(item->AbsolutePath);
        }

        if (ImGui::MenuItem("Rename"))
        {
            RequestRename(item->AbsolutePath);
        }

        ImGui::Separator();

        if (ImGui::MenuItem(GetDeleteMenuLabel(item)))
        {
            RequestDelete(item->AbsolutePath);
        }

        ImGui::EndPopup();
    }
}

void QEProjectBrowserActions::DrawAssetTileContextMenu(QEProjectAssetItem* item)
{
    if (item == nullptr)
        return;

    if (ImGui::BeginPopupContextItem())
    {
        if (item->IsDirectory)
        {
            if (ImGui::MenuItem("New Folder"))
            {
                TryCreateNewFolderInPath(item->AbsolutePath);
            }

            ImGui::Separator();
        }

        if (ImGui::MenuItem("Rename"))
        {
            RequestRename(item->AbsolutePath);
        }

        if (ImGui::MenuItem(GetDeleteMenuLabel(item)))
        {
            RequestDelete(item->AbsolutePath);
        }

        ImGui::EndPopup();
    }
}

void QEProjectBrowserActions::DrawContentsBackgroundContextMenu(QEProjectAssetItem* folder)
{
    if (folder == nullptr || !folder->IsDirectory)
        return;

    if (ImGui::BeginPopupContextWindow(
        "ContentsBackgroundContextMenu",
        ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
    {
        if (ImGui::MenuItem("New Folder"))
        {
            TryCreateNewFolderInPath(folder->AbsolutePath);
        }

        ImGui::EndPopup();
    }
}

void QEProjectBrowserActions::DrawRenamePopup()
{
    const char* popupTitle = GetRenamePopupTitle(_renamePopupTargetItem);

    if (_openRenamePopupNextFrame)
    {
        ImGui::OpenPopup(popupTitle);
        _openRenamePopupNextFrame = false;
    }

    if (ImGui::BeginPopupModal(popupTitle, nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::TextUnformatted(GetRenamePopupLabel(_renamePopupTargetItem));
        ImGui::Spacing();
        ImGui::TextWrapped("%s", _renamePopupTargetPath.string().c_str());
        ImGui::Spacing();

        ImGui::SetNextItemWidth(320.0f);
        const bool enterPressed = ImGui::InputText(
            "New name",
            _renamePopupBuffer.data(),
            _renamePopupBuffer.size(),
            ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);

        if (enterPressed)
        {
            if (ExecuteRename())
            {
                ImGui::CloseCurrentPopup();
            }
        }

        ImGui::Spacing();

        if (ImGui::Button("Rename", ImVec2(120, 0)))
        {
            if (ExecuteRename())
            {
                ImGui::CloseCurrentPopup();
            }
        }

        ImGui::SameLine();

        if (ImGui::Button("Cancel", ImVec2(120, 0)))
        {
            _renamePopupTargetPath.clear();
            _renamePopupTargetItem = nullptr;
            _renamePopupBuffer.fill('\0');
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}
