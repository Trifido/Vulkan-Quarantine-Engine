#include "ShadowMappingModule.h"
#include "ImageMemoryTools.h"

ShadowMappingModule* ShadowMappingModule::instance = nullptr;

ShadowMappingModule::ShadowMappingModule()
{
    this->TextureSize = 1024;
    this->shadowFormat = VK_FORMAT_D32_SFLOAT;
}

void ShadowMappingModule::InitializeShadowMapPipeline(std::shared_ptr<ShadowPipelineModule> shadowPipelineModule)
{
    this->shadowPipelineModule = shadowPipelineModule;
}

void ShadowMappingModule::CreateShadowMapResources()
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
}

void ShadowMappingModule::cleanup()
{
    TextureManagerModule::cleanup();
    vkDestroySampler(this->deviceModule->device, this->depthSampler, nullptr);
}

ShadowMappingModule* ShadowMappingModule::getInstance()
{
    if (instance == NULL)
        instance = new ShadowMappingModule();

    return instance;
}

void ShadowMappingModule::ResetInstance()
{
    delete instance;
    instance = nullptr;
}
