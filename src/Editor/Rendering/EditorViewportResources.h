#pragma once

#include <vulkan/vulkan.h>
#include <imgui.h>
#include <QERenderTarget.h>

class DeviceModule;
class RenderPassModule;
class CommandPoolModule;
class QueueModule;

class EditorViewportResources
{
public:
    EditorViewportResources();
    ~EditorViewportResources();

    void Initialize(
        DeviceModule* deviceModule,
        RenderPassModule* renderPassModule,
        CommandPoolModule* commandPoolModule,
        QueueModule* queueModule);
    void Cleanup();

    void Resize(uint32_t width, uint32_t height);

    bool IsValid() const;
    bool NeedsResize(uint32_t width, uint32_t height) const;

    const QERenderTarget& GetRenderTarget() const { return renderTarget; }
    ImTextureID GetImGuiTexture() const { return (ImTextureID)imguiDescriptorSet; }

    void Rebuild();

private:
    void CleanupImages();
    void CreateImages();
    void CreateFramebuffer();
    void RegisterImGuiTexture();
    void UnregisterImGuiTexture();
    void TransitionImageLayout(
        VkImage image,
        VkImageLayout oldLayout,
        VkImageLayout newLayout,
        VkImageSubresourceRange range);

private:
    DeviceModule* deviceModule = nullptr;
    RenderPassModule* renderPassModule = nullptr;
    CommandPoolModule* commandPoolModule = nullptr;
    QueueModule* queueModule = nullptr;

    QERenderTarget renderTarget{};

    VkImage msaaColorImage = VK_NULL_HANDLE;
    VkDeviceMemory msaaColorMemory = VK_NULL_HANDLE;
    VkImageView msaaColorImageView = VK_NULL_HANDLE;

    VkImage resolveImage = VK_NULL_HANDLE;
    VkDeviceMemory resolveMemory = VK_NULL_HANDLE;
    VkImageView resolveImageView = VK_NULL_HANDLE;
    VkSampler resolveSampler = VK_NULL_HANDLE;

    VkImage depthImage = VK_NULL_HANDLE;
    VkDeviceMemory depthMemory = VK_NULL_HANDLE;
    VkImageView depthImageView = VK_NULL_HANDLE;

    VkDescriptorSet imguiDescriptorSet = VK_NULL_HANDLE;
};
