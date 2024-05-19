#include "ShadowMappingModule.h"
#include "ImageMemoryTools.h"
#include "FrameBufferModule.h"

ShadowMappingModule::ShadowMappingModule()
{
    this->TextureSize = 1024;
    this->shadowFormat = VK_FORMAT_D32_SFLOAT;
}

ShadowMappingModule::ShadowMappingModule(std::shared_ptr<ShaderModule> shaderModule, VkRenderPass& renderPass, ShadowMappingMode mode) : ShadowMappingModule()
{
    this->shaderModule = shaderModule;
    this->shadowPipelineModule = this->shaderModule->ShadowPipelineModule;
    this->shadowMode = mode;

    switch (this->shadowMode)
    {
        default:
        case ShadowMappingMode::NONE:
            break;
        case ShadowMappingMode::DIRECTIONAL_SHADOW:
            this->CreateDirectionalShadowMapResources(renderPass);
            break;
        case ShadowMappingMode::OMNI_SHADOW:
            this->shadowFormat = VK_FORMAT_R32_SFLOAT;
            this->CreateOmniShadowMapResources(renderPass);
            break;
        case ShadowMappingMode::CASCADE_SHADOW:
            break;
    }
}

void ShadowMappingModule::CreateDirectionalShadowMapResources(VkRenderPass& renderPass)
{
    this->createImage(this->TextureSize, this->TextureSize, this->shadowFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 1);
    this->imageView = IMT::createImageView(this->deviceModule->device, this->image, this->shadowFormat, VK_IMAGE_ASPECT_DEPTH_BIT);

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = samplerInfo.addressModeU;
    samplerInfo.addressModeW = samplerInfo.addressModeU;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 1.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

    if(vkCreateSampler(this->deviceModule->device, &samplerInfo, nullptr, &this->depthSampler))
    {
        throw std::runtime_error("failed to create shadow texture sampler!");
    }

    this->shadowFrameBuffer = FramebufferModule::CreateShadowFramebuffer(renderPass, this->imageView, this->TextureSize, this->deviceModule->device);
}

void ShadowMappingModule::CreateOmniShadowMapResources(VkRenderPass& renderPass)
{
    this->createCubeMapImage(this->TextureSize, this->TextureSize, this->shadowFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 1);
    this->createCubeMapImageViews = IMT::createCubeMapImageView(this->deviceModule->device, this->image, this->shadowFormat, VK_IMAGE_ASPECT_COLOR_BIT);

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = samplerInfo.addressModeU;
    samplerInfo.addressModeW = samplerInfo.addressModeU;
    samplerInfo.compareOp = VK_COMPARE_OP_NEVER;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 1.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

    if (vkCreateSampler(this->deviceModule->device, &samplerInfo, nullptr, &this->depthSampler))
    {
        throw std::runtime_error("failed to create omni shadow texture sampler!");
    }

    this->shadowFrameBuffer = FramebufferModule::CreateShadowFramebuffer(renderPass, this->imageView, this->TextureSize, this->deviceModule->device);
}

void ShadowMappingModule::cleanup()
{
    TextureManagerModule::cleanup();
    vkDestroySampler(this->deviceModule->device, this->depthSampler, nullptr);

    vkDestroyFramebuffer(this->deviceModule->device, this->shadowFrameBuffer, nullptr);
}
