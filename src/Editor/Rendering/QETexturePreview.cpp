#include "QETexturePreview.h"

#include <backends/imgui_impl_vulkan.h>
#include <CustomTexture.h>

QETexturePreview::QETexturePreview() = default;
QETexturePreview::~QETexturePreview()
{
    Cleanup();
}

bool QETexturePreview::LoadFromFile(const std::string& path)
{
    Cleanup();

    try
    {
        _texture = std::make_unique<CustomTexture>(
            path,
            TEXTURE_TYPE::DIFFUSE_TYPE,
            QEColorSpace::SRGB);

        _descriptorSet = ImGui_ImplVulkan_AddTexture(
            _texture->textureSampler,
            _texture->imageView,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        if (_descriptorSet == VK_NULL_HANDLE)
        {
            Cleanup();
            return false;
        }

        _imguiTexture = (ImTextureID)_descriptorSet;
        _width = _texture->GetWidth();
        _height = _texture->GetHeight();
        _mipLevels = _texture->GetMipLevels();

        return true;
    }
    catch (...)
    {
        Cleanup();
        return false;
    }
}

void QETexturePreview::Cleanup()
{
    if (_descriptorSet != VK_NULL_HANDLE)
    {
        ImGui_ImplVulkan_RemoveTexture(_descriptorSet);
        _descriptorSet = VK_NULL_HANDLE;
    }

    _imguiTexture = 0;
    _width = 0;
    _height = 0;
    _mipLevels = 1;

    if (_texture)
    {
        _texture->cleanup();
        _texture.reset();
    }
}
