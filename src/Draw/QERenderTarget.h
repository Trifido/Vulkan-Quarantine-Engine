#pragma once

#include <vulkan/vulkan.h>

struct QERenderTarget
{
    VkFramebuffer Framebuffer = VK_NULL_HANDLE;
    VkRenderPass RenderPass = VK_NULL_HANDLE;
    VkExtent2D Extent{ 0, 0 };

    bool Valid() const
    {
        return Framebuffer != VK_NULL_HANDLE &&
            RenderPass != VK_NULL_HANDLE &&
            Extent.width > 0 &&
            Extent.height > 0;
    }
};
