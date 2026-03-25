#include "QEProjectBrowserPanel.h"

#include <algorithm>
#include <cstring>
#include <imgui.h>
#include <imgui_impl_vulkan.h>

#include <DeviceModule.h>
#include <QueueModule.h>
#include <CommandPoolModule.h>
#include <ImageMemoryTools.h>
#include <SyncTool.h>

#include <stb_image.h>
#include <QEProjectManager.h>

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

    if (ext == ".qescene") return QEAssetType::Scene;
    if (ext == ".qemat")   return QEAssetType::Material;
    if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".ktx2") return QEAssetType::Texture;
    if (ext == ".gltf") return QEAssetType::Mesh;
    if (ext == ".glb") return QEAssetType::Animation;
    if (ext == ".bin") return QEAssetType::Unknown;

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

QEProjectBrowserPanel::~QEProjectBrowserPanel()
{
    CleanupIcons();
}

bool QEProjectBrowserPanel::InitializeIcons()
{
    CleanupIcons();

    bool ok = true;
    ok &= LoadIconTexture(QEAssetType::Folder, "../../src/Editor/Icons/folder.png");
    ok &= LoadIconTexture(QEAssetType::Scene, "../../src/Editor/Icons/scene.png");
    ok &= LoadIconTexture(QEAssetType::Material, "../../src/Editor/Icons/material.png");
    ok &= LoadIconTexture(QEAssetType::Texture, "../../src/Editor/Icons/texture.png");
    ok &= LoadIconTexture(QEAssetType::Mesh, "../../src/Editor/Icons/mesh.png");
    ok &= LoadIconTexture(QEAssetType::Animation, "../../src/Editor/Icons/animation.png");
    return ok;
}

const QEIconTexture* QEProjectBrowserPanel::GetAssetIconTexture(QEAssetType type) const
{
    auto it = _iconTextures.find(type);
    if (it == _iconTextures.end())
        return nullptr;

    return it->second.IsValid ? &it->second : nullptr;
}

bool QEProjectBrowserPanel::LoadIconTexture(QEAssetType type, const std::filesystem::path& filePath)
{
    int texWidth = 0;
    int texHeight = 0;
    int texChannels = 0;

    stbi_uc* pixels = stbi_load(filePath.string().c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    if (!pixels || texWidth <= 0 || texHeight <= 0)
        return false;

    const VkDeviceSize imageSize = static_cast<VkDeviceSize>(texWidth) * static_cast<VkDeviceSize>(texHeight) * 4;
    auto device = DeviceModule::getInstance();

    QEIconTexture icon{};
    icon.Width = static_cast<uint32_t>(texWidth);
    icon.Height = static_cast<uint32_t>(texHeight);

    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory stagingBufferMemory = VK_NULL_HANDLE;

    if (!CreateBuffer(
        imageSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer,
        stagingBufferMemory))
    {
        stbi_image_free(pixels);
        return false;
    }

    void* mappedData = nullptr;
    if (vkMapMemory(device->device, stagingBufferMemory, 0, imageSize, 0, &mappedData) != VK_SUCCESS)
    {
        vkDestroyBuffer(device->device, stagingBuffer, nullptr);
        vkFreeMemory(device->device, stagingBufferMemory, nullptr);
        stbi_image_free(pixels);
        return false;
    }

    std::memcpy(mappedData, pixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(device->device, stagingBufferMemory);
    stbi_image_free(pixels);

    if (!CreateImage(
        static_cast<uint32_t>(texWidth),
        static_cast<uint32_t>(texHeight),
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        icon.Image,
        icon.Memory))
    {
        vkDestroyBuffer(device->device, stagingBuffer, nullptr);
        vkFreeMemory(device->device, stagingBufferMemory, nullptr);
        return false;
    }

    TransitionImageLayout(
        icon.Image,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    CopyBufferToImage(
        stagingBuffer,
        icon.Image,
        static_cast<uint32_t>(texWidth),
        static_cast<uint32_t>(texHeight));

    TransitionImageLayout(
        icon.Image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vkDestroyBuffer(device->device, stagingBuffer, nullptr);
    vkFreeMemory(device->device, stagingBufferMemory, nullptr);

    icon.ImageView = CreateImageView(
        icon.Image,
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_ASPECT_COLOR_BIT);

    if (icon.ImageView == VK_NULL_HANDLE)
    {
        vkDestroyImage(device->device, icon.Image, nullptr);
        vkFreeMemory(device->device, icon.Memory, nullptr);
        return false;
    }

    icon.Sampler = CreateSampler();
    if (icon.Sampler == VK_NULL_HANDLE)
    {
        vkDestroyImageView(device->device, icon.ImageView, nullptr);
        vkDestroyImage(device->device, icon.Image, nullptr);
        vkFreeMemory(device->device, icon.Memory, nullptr);
        return false;
    }

    icon.DescriptorSet = ImGui_ImplVulkan_AddTexture(
        icon.Sampler,
        icon.ImageView,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    icon.ImGuiTexture = (ImTextureID)icon.DescriptorSet;
    icon.IsValid = (icon.DescriptorSet != VK_NULL_HANDLE);

    if (!icon.IsValid)
    {
        vkDestroySampler(device->device, icon.Sampler, nullptr);
        vkDestroyImageView(device->device, icon.ImageView, nullptr);
        vkDestroyImage(device->device, icon.Image, nullptr);
        vkFreeMemory(device->device, icon.Memory, nullptr);
        return false;
    }

    _iconTextures[type] = icon;
    return true;
}

uint32_t QEProjectBrowserPanel::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const
{
    auto device = DeviceModule::getInstance();
    return IMT::findMemoryType(typeFilter, properties, device->physicalDevice);
}

bool QEProjectBrowserPanel::CreateBuffer(
    VkDeviceSize size,
    VkBufferUsageFlags usage,
    VkMemoryPropertyFlags properties,
    VkBuffer& buffer,
    VkDeviceMemory& bufferMemory)
{
    auto device = DeviceModule::getInstance();

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device->device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
        return false;

    VkMemoryRequirements memRequirements{};
    vkGetBufferMemoryRequirements(device->device, buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(device->device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
    {
        vkDestroyBuffer(device->device, buffer, nullptr);
        buffer = VK_NULL_HANDLE;
        return false;
    }

    vkBindBufferMemory(device->device, buffer, bufferMemory, 0);
    return true;
}

bool QEProjectBrowserPanel::CreateImage(
    uint32_t width,
    uint32_t height,
    VkFormat format,
    VkImageTiling tiling,
    VkImageUsageFlags usage,
    VkMemoryPropertyFlags properties,
    VkImage& image,
    VkDeviceMemory& imageMemory)
{
    auto device = DeviceModule::getInstance();

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent = { width, height, 1 };
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(device->device, &imageInfo, nullptr, &image) != VK_SUCCESS)
        return false;

    VkMemoryRequirements memRequirements{};
    vkGetImageMemoryRequirements(device->device, image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(device->device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS)
    {
        vkDestroyImage(device->device, image, nullptr);
        image = VK_NULL_HANDLE;
        return false;
    }

    vkBindImageMemory(device->device, image, imageMemory, 0);
    return true;
}

void QEProjectBrowserPanel::TransitionImageLayout(
    VkImage image,
    VkImageLayout oldLayout,
    VkImageLayout newLayout)
{
    auto device = DeviceModule::getInstance();
    auto commandPool = CommandPoolModule::getInstance();
    auto queueModule = QueueModule::getInstance();

    VkCommandBuffer commandBuffer = beginSingleTimeCommands(
        device->device,
        commandPool->getCommandPool());

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    VkPipelineStageFlags destinationStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else
    {
        endSingleTimeCommands(
            device->device,
            queueModule->graphicsQueue,
            commandPool->getCommandPool(),
            commandBuffer);

        throw std::runtime_error("Unsupported image layout transition in QEProjectBrowserPanel.");
    }

    vkCmdPipelineBarrier(
        commandBuffer,
        sourceStage,
        destinationStage,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier);

    endSingleTimeCommands(
        device->device,
        queueModule->graphicsQueue,
        commandPool->getCommandPool(),
        commandBuffer);
}

void QEProjectBrowserPanel::CopyBufferToImage(
    VkBuffer buffer,
    VkImage image,
    uint32_t width,
    uint32_t height)
{
    auto device = DeviceModule::getInstance();
    auto commandPool = CommandPoolModule::getInstance();
    auto queueModule = QueueModule::getInstance();

    VkCommandBuffer commandBuffer = beginSingleTimeCommands(
        device->device,
        commandPool->getCommandPool());

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = { 0, 0, 0 };
    region.imageExtent = { width, height, 1 };

    vkCmdCopyBufferToImage(
        commandBuffer,
        buffer,
        image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &region);

    endSingleTimeCommands(
        device->device,
        queueModule->graphicsQueue,
        commandPool->getCommandPool(),
        commandBuffer);
}

VkImageView QEProjectBrowserPanel::CreateImageView(
    VkImage image,
    VkFormat format,
    VkImageAspectFlags aspectFlags)
{
    auto device = DeviceModule::getInstance();

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = aspectFlags;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VkImageView imageView = VK_NULL_HANDLE;
    if (vkCreateImageView(device->device, &viewInfo, nullptr, &imageView) != VK_SUCCESS)
        return VK_NULL_HANDLE;

    return imageView;
}

VkSampler QEProjectBrowserPanel::CreateSampler()
{
    auto device = DeviceModule::getInstance();

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;
    samplerInfo.mipLodBias = 0.0f;

    VkSampler sampler = VK_NULL_HANDLE;
    if (vkCreateSampler(device->device, &samplerInfo, nullptr, &sampler) != VK_SUCCESS)
        return VK_NULL_HANDLE;

    return sampler;
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
            if (!ShouldDisplayPath(entry.path()))
                continue;

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
        _selectedFolder = _rootItem.get();
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
        "%s",
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

    std::string headerLabel = "Contents";

    if (_selectedItem != nullptr)
    {
        headerLabel = _selectedItem->Name;
    }
    else if (folder != nullptr)
    {
        headerLabel = folder->Name;
    }

    ImGui::TextUnformatted(headerLabel.c_str());
    ImGui::Spacing();

    const float availableWidth = ImGui::GetContentRegionAvail().x;
    int columnCount = static_cast<int>(availableWidth / (tileSize + cellPadding));

    if (columnCount < 1)
        columnCount = 1;

    if (ImGui::BeginTable("ProjectBrowserGrid", columnCount, ImGuiTableFlags_SizingFixedFit))
    {
        for (const auto& child : folder->Children)
        {
            ImGui::TableNextColumn();
            DrawAssetTile(child.get(), tileSize);
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
    const float iconAreaSize = tileSize;
    const float labelHeight = 28.0f;
    const float totalHeight = iconAreaSize + labelHeight;

    ImVec2 cursorPos = ImGui::GetCursorScreenPos();
    ImVec2 tileMin = cursorPos;
    ImVec2 iconAreaMax = ImVec2(cursorPos.x + iconAreaSize, cursorPos.y + iconAreaSize);

    ImGui::InvisibleButton("##tile", ImVec2(iconAreaSize, totalHeight));

    const bool hovered = ImGui::IsItemHovered();
    const bool clicked = ImGui::IsItemClicked();

    if (clicked)
        _selectedItem = item;

    if (hovered && ImGui::IsMouseDoubleClicked(0) && item->IsDirectory)
    {
        _selectedFolder = item;
        _selectedItem = nullptr;
    }

    if (IsItemDraggable(item))
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

    const QEIconTexture* iconTexture = GetAssetIconTexture(item->Type);

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
        const char* icon = GetAssetIcon(item->Type);
        ImVec4 iconColor = GetAssetColor(item->Type);

        ImVec2 iconTextSize = ImGui::CalcTextSize(icon);
        float iconX = tileMin.x + (iconAreaSize - iconTextSize.x) * 0.5f;
        float iconY = tileMin.y + (iconAreaSize - iconTextSize.y) * 0.5f;

        drawList->AddText(
            ImVec2(iconX, iconY),
            ImGui::ColorConvertFloat4ToU32(iconColor),
            icon);
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

    ImGui::PopID();
}

bool QEProjectBrowserPanel::ShouldDisplayPath(const std::filesystem::path& path) const
{
    if (std::filesystem::is_directory(path))
        return true;

    std::string ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    if (ext == ".bin")
        return false;

    return true;
}

bool QEProjectBrowserPanel::IsItemDraggable(const QEProjectAssetItem* item) const
{
    if (item == nullptr || item->IsDirectory)
        return false;

    std::filesystem::path p(item->AbsolutePath);
    std::string ext = p.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    return ext == ".gltf";
}

std::string QEProjectBrowserPanel::GetDisplayAssetName(const QEProjectAssetItem* item) const
{
    if (item == nullptr)
        return "";

    if (item->IsDirectory)
        return item->Name;

    std::filesystem::path p(item->Name);
    return p.stem().string();
}

void QEProjectBrowserPanel::Draw()
{
    _isWindowHovered = false;
    _isContentsPanelHovered = false;

    if (!ImGui::Begin("Project Browser"))
    {
        ImGui::End();
        return;
    }

    _isWindowHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows);

    if (ImGui::Button("Refresh"))
        Refresh();

    ImGui::Spacing();

    if (_rootItem == nullptr)
    {
        ImGui::TextUnformatted("No project loaded.");
        HandleExternalFileDrops();
        ImGui::End();
        return;
    }

    ImGuiTableFlags tableFlags =
        ImGuiTableFlags_Resizable |
        ImGuiTableFlags_SizingStretchProp |
        ImGuiTableFlags_BordersInnerV;

    if (ImGui::BeginTable("ProjectBrowserLayout", 2, tableFlags))
    {
        ImGui::TableSetupColumn("Folders", ImGuiTableColumnFlags_WidthStretch, 0.32f);
        ImGui::TableSetupColumn("Contents", ImGuiTableColumnFlags_WidthStretch, 0.68f);

        ImGui::TableNextColumn();

        ImGui::TextUnformatted("Folders");
        ImGui::Spacing();

        if (ImGui::BeginChild("FoldersPanel", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar))
        {
            DrawFolderTree(_rootItem.get());
        }
        ImGui::EndChild();

        ImGui::TableNextColumn();

        ImGui::TextUnformatted("Contents");
        ImGui::Spacing();

        if (ImGui::BeginChild("ContentsPanel", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar))
        {
            _isContentsPanelHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows);
            DrawFolderContents(_selectedFolder);
        }
        ImGui::EndChild();

        ImGui::EndTable();
    }

    HandleExternalFileDrops();

    ImGui::End();
}

void QEProjectBrowserPanel::CleanupIcons()
{
    auto device = DeviceModule::getInstance();

    for (auto& [type, icon] : _iconTextures)
    {
        if (icon.DescriptorSet != VK_NULL_HANDLE)
        {
            ImGui_ImplVulkan_RemoveTexture(icon.DescriptorSet);
            icon.DescriptorSet = VK_NULL_HANDLE;
            icon.ImGuiTexture = 0;
        }

        if (icon.Sampler != VK_NULL_HANDLE)
        {
            vkDestroySampler(device->device, icon.Sampler, nullptr);
            icon.Sampler = VK_NULL_HANDLE;
        }

        if (icon.ImageView != VK_NULL_HANDLE)
        {
            vkDestroyImageView(device->device, icon.ImageView, nullptr);
            icon.ImageView = VK_NULL_HANDLE;
        }

        if (icon.Image != VK_NULL_HANDLE)
        {
            vkDestroyImage(device->device, icon.Image, nullptr);
            icon.Image = VK_NULL_HANDLE;
        }

        if (icon.Memory != VK_NULL_HANDLE)
        {
            vkFreeMemory(device->device, icon.Memory, nullptr);
            icon.Memory = VK_NULL_HANDLE;
        }
    }

    _iconTextures.clear();
}

void QEProjectBrowserPanel::SetPendingExternalDrops(const std::vector<std::filesystem::path>& paths)
{
    _pendingExternalDrops = paths;
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

void QEProjectBrowserPanel::HandleExternalFileDrops()
{
    if (_pendingExternalDrops.empty())
        return;

    if (!_isWindowHovered && !_isContentsPanelHovered)
        return;

    bool importedAny = false;

    for (const auto& droppedPath : _pendingExternalDrops)
    {
        if (!IsImportableExternalFile(droppedPath))
            continue;

        try
        {
            QEProjectManager::ImportMeshFile(droppedPath.string());
            importedAny = true;
        }
        catch (const std::exception& e)
        {
            std::cerr << "Import failed: " << e.what() << std::endl;
        }
    }

    _pendingExternalDrops.clear();

    if (importedAny)
        Refresh();
}
