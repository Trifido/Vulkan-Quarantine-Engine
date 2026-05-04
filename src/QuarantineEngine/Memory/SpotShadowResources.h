#pragma once

#ifndef SPOT_SHADOW_RESOURCES_H
#define SPOT_SHADOW_RESOURCES_H

#include <vulkan/vulkan.hpp>
#include <DeviceModule.h>
#include <SwapChainModule.h>
#include <CSMResources.h>

class SpotShadowResources
{
private:
    DeviceModule* deviceModule = nullptr;
    SwapChainModule* swapchainModule = nullptr;

    VkImage shadowImage = VK_NULL_HANDLE;
    VkDeviceMemory shadowImageMemory = VK_NULL_HANDLE;

    void CreateSpotShadowResources(std::shared_ptr<VkRenderPass> renderPass);

public:
    static uint32_t TextureSize;

    VkFormat shadowFormat = VK_FORMAT_UNDEFINED;
    float DepthBiasConstant = 0.10f;
    float DepthBiasSlope = 0.50f;

    std::shared_ptr<UniformBufferObject> OffscreenShadowMapUBO = nullptr;
    VkImageView ShadowImageView = VK_NULL_HANDLE;
    VkSampler ShadowSampler = VK_NULL_HANDLE;
    VkFramebuffer frameBuffer = VK_NULL_HANDLE;
    glm::mat4 ViewProjMatrix = glm::mat4(1.0f);

public:
    SpotShadowResources();
    SpotShadowResources(std::shared_ptr<VkRenderPass> renderPass);

    void UpdateOffscreenUBOShadowMap();
    void Cleanup();
};



namespace QE
{
    using ::SpotShadowResources;
} // namespace QE
// QE namespace aliases
#endif
