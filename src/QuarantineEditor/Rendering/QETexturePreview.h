#pragma once

#include <memory>
#include <string>
#include <imgui.h>
#include <vulkan/vulkan.h>

class CustomTexture;

class QETexturePreview
{
public:
    QETexturePreview();
    ~QETexturePreview();

    QETexturePreview(const QETexturePreview&) = delete;
    QETexturePreview& operator=(const QETexturePreview&) = delete;

    bool LoadFromFile(const std::string& path);
    void Cleanup();

    bool IsValid() const { return _imguiTexture != 0; }
    ImTextureID GetImGuiTexture() const { return _imguiTexture; }

    int GetWidth() const { return _width; }
    int GetHeight() const { return _height; }
    uint32_t GetMipLevels() const { return _mipLevels; }

private:
    std::unique_ptr<CustomTexture> _texture;
    VkDescriptorSet _descriptorSet = VK_NULL_HANDLE;
    ImTextureID _imguiTexture = 0;

    int _width = 0;
    int _height = 0;
    uint32_t _mipLevels = 1;
};
