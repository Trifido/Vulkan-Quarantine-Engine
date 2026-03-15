#pragma once

#include <vulkan/vulkan.h>
#include <imgui.h>
#include <QERenderTarget.h>

class DeviceModule;
class RenderPassModule;

class EditorViewportResources
{
public:
    EditorViewportResources();
    ~EditorViewportResources();

    void Initialize(DeviceModule* deviceModule, RenderPassModule* renderPassModule);
    void Cleanup();

    void Resize(uint32_t width, uint32_t height);

    bool IsValid() const;
    bool NeedsResize(uint32_t width, uint32_t height) const;

    const QERenderTarget& GetRenderTarget() const { return renderTarget; }
    ImTextureID GetImGuiTexture() const { return (ImTextureID)imguiDescriptorSet; }

private:
    void CleanupImages();
    void CreateImages();
    void CreateFramebuffer();
    void RegisterImGuiTexture();
    void UnregisterImGuiTexture();

private:
    DeviceModule* deviceModule = nullptr;
    RenderPassModule* renderPassModule = nullptr;

    QERenderTarget renderTarget{};

    VkImage colorImage = VK_NULL_HANDLE;
    VkDeviceMemory colorMemory = VK_NULL_HANDLE;
    VkImageView colorImageView = VK_NULL_HANDLE;
    VkSampler colorSampler = VK_NULL_HANDLE;

    VkImage depthImage = VK_NULL_HANDLE;
    VkDeviceMemory depthMemory = VK_NULL_HANDLE;
    VkImageView depthImageView = VK_NULL_HANDLE;

    VkDescriptorSet imguiDescriptorSet = VK_NULL_HANDLE;
};
