#pragma once

#include "IEditorPanel.h"
#include "Editor/Browser/QEProjectAssetItem.h"
#include <string>
#include <memory>
#include <filesystem>
#include <imgui.h>
#include <vulkan/vulkan.h>
#include <unordered_map>

constexpr float tileSize = 92.0f;
constexpr float cellPadding = 12.0f;
constexpr float textPadding = 2.0f;

struct QEIconTexture
{
    VkImage Image = VK_NULL_HANDLE;
    VkDeviceMemory Memory = VK_NULL_HANDLE;
    VkImageView ImageView = VK_NULL_HANDLE;
    VkSampler Sampler = VK_NULL_HANDLE;

    VkDescriptorSet DescriptorSet = VK_NULL_HANDLE;
    ImTextureID ImGuiTexture = 0;

    uint32_t Width = 0;
    uint32_t Height = 0;
    bool IsValid = false;
};

class QEProjectBrowserPanel : public IEditorPanel
{
public:
    QEProjectBrowserPanel() = default;
    ~QEProjectBrowserPanel() override;

    bool InitializeIcons();
    void SetProjectRootPath(const std::filesystem::path& projectRootPath);
    const std::filesystem::path& GetProjectRootPath() const { return _projectRootPath; }
    void Refresh();
    void Draw() override;

    void CleanupIcons();

    const char* GetName() const override { return "Asset Browser"; }
    void SetPendingExternalDrops(const std::vector<std::filesystem::path>& paths);

private:
    std::filesystem::path _projectRootPath;
    std::shared_ptr<QEProjectAssetItem> _rootItem = nullptr;

    std::unordered_map<QEAssetType, QEIconTexture> _iconTextures;

    QEProjectAssetItem* _selectedFolder = nullptr;
    QEProjectAssetItem* _selectedItem = nullptr;

    const float iconAreaSize = tileSize;
    const float labelHeight = 32.0f;
    const float totalHeight = iconAreaSize + labelHeight;

    std::vector<std::filesystem::path> _pendingExternalDrops;
    bool _isWindowHovered = false;
    bool _isContentsPanelHovered = false;

private:
    std::shared_ptr<QEProjectAssetItem> BuildDirectoryRecursive(
        const std::filesystem::path& path,
        QEProjectAssetItem* parent);

    QEAssetType GetAssetTypeFromPath(const std::filesystem::path& path) const;
    std::string GetDisplayNameFitted(const std::string& name, float maxWidth) const;

    void DrawFolderTree(QEProjectAssetItem* item);
    void DrawFolderContents(QEProjectAssetItem* folder);
    void DrawAssetTile(QEProjectAssetItem* item, float tileSize);
    bool ShouldDisplayPath(const std::filesystem::path& path) const;
    bool IsItemDraggable(const QEProjectAssetItem* item) const;

    std::string GetDisplayAssetName(const QEProjectAssetItem* item) const;
    const char* GetAssetIcon(QEAssetType type) const;
    ImVec4 GetAssetColor(QEAssetType type) const;

    const QEIconTexture* GetAssetIconTexture(QEAssetType type) const;
    bool LoadIconTexture(QEAssetType type, const std::filesystem::path& filePath);

    void HandleExternalFileDrops();
    bool IsImportableExternalFile(const std::filesystem::path& path) const;

private:
    bool CreateBuffer(
        VkDeviceSize size,
        VkBufferUsageFlags usage,
        VkMemoryPropertyFlags properties,
        VkBuffer& buffer,
        VkDeviceMemory& bufferMemory);

    bool CreateImage(
        uint32_t width,
        uint32_t height,
        VkFormat format,
        VkImageTiling tiling,
        VkImageUsageFlags usage,
        VkMemoryPropertyFlags properties,
        VkImage& image,
        VkDeviceMemory& imageMemory);

    void TransitionImageLayout(
        VkImage image,
        VkImageLayout oldLayout,
        VkImageLayout newLayout);

    void CopyBufferToImage(
        VkBuffer buffer,
        VkImage image,
        uint32_t width,
        uint32_t height);

    VkImageView CreateImageView(
        VkImage image,
        VkFormat format,
        VkImageAspectFlags aspectFlags);

    VkSampler CreateSampler();
    uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;
};
