#include "CustomTexture.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "BufferManageModule.h"
#include "SyncTool.h"
#include <unordered_map>
#include <Helpers/ScopedTimer.h>
#include <ktxvulkan.h>
#include <filesystem>


VkCommandPool CustomTexture::commandPool;

CustomTexture::CustomTexture()
{
    this->type = TEXTURE_TYPE::NULL_TYPE;
    this->createTextureImage("");
}

CustomTexture::CustomTexture(std::string path, TEXTURE_TYPE type)
{
    this->type = type;
    this->texturePaths.push_back(path);

    if (type == TEXTURE_TYPE::CUBEMAP_TYPE)
    {
        this->createCubemapTextureImage(path);
    }
    else
    {
        this->createTextureImage(path);
    }
}

CustomTexture::CustomTexture(std::vector<std::string> path)
{
    this->texturePaths = path;
    this->type = TEXTURE_TYPE::CUBEMAP_TYPE;
    this->createCubemapTextureImage(path);
}

CustomTexture::CustomTexture(aiTexel* data, unsigned int width, unsigned int height, TEXTURE_TYPE type)
{
    this->type = type;
    this->createTextureRawImage(data, width, height);
}

int CustomTexture::GetChannelCount(VkFormat format) {
    static const std::unordered_map<VkFormat, int> formatChannels = {
        {VK_FORMAT_R8_UNORM, 1}, {VK_FORMAT_R8_SNORM, 1}, {VK_FORMAT_R8_UINT, 1}, {VK_FORMAT_R8_SINT, 1},
        {VK_FORMAT_R16_UNORM, 1}, {VK_FORMAT_R16_SNORM, 1}, {VK_FORMAT_R16_UINT, 1}, {VK_FORMAT_R16_SINT, 1}, {VK_FORMAT_R16_SFLOAT, 1},
        {VK_FORMAT_R32_UINT, 1}, {VK_FORMAT_R32_SINT, 1}, {VK_FORMAT_R32_SFLOAT, 1},

        {VK_FORMAT_R8G8_UNORM, 2}, {VK_FORMAT_R8G8_SNORM, 2}, {VK_FORMAT_R8G8_UINT, 2}, {VK_FORMAT_R8G8_SINT, 2},
        {VK_FORMAT_R16G16_UNORM, 2}, {VK_FORMAT_R16G16_SNORM, 2}, {VK_FORMAT_R16G16_UINT, 2}, {VK_FORMAT_R16G16_SINT, 2}, {VK_FORMAT_R16G16_SFLOAT, 2},
        {VK_FORMAT_R32G32_UINT, 2}, {VK_FORMAT_R32G32_SINT, 2}, {VK_FORMAT_R32G32_SFLOAT, 2},

        {VK_FORMAT_R8G8B8_UNORM, 3}, {VK_FORMAT_R8G8B8_SNORM, 3}, {VK_FORMAT_R8G8B8_UINT, 3}, {VK_FORMAT_R8G8B8_SINT, 3},
        {VK_FORMAT_R16G16B16_UNORM, 3}, {VK_FORMAT_R16G16B16_SNORM, 3}, {VK_FORMAT_R16G16B16_UINT, 3}, {VK_FORMAT_R16G16B16_SINT, 3}, {VK_FORMAT_R16G16B16_SFLOAT, 3},
        {VK_FORMAT_R32G32B32_UINT, 3}, {VK_FORMAT_R32G32B32_SINT, 3}, {VK_FORMAT_R32G32B32_SFLOAT, 3},

        {VK_FORMAT_R8G8B8A8_UNORM, 4}, {VK_FORMAT_R8G8B8A8_SNORM, 4}, {VK_FORMAT_R8G8B8A8_UINT, 4}, {VK_FORMAT_R8G8B8A8_SINT, 4},
        {VK_FORMAT_R16G16B16A16_UNORM, 4}, {VK_FORMAT_R16G16B16A16_SNORM, 4}, {VK_FORMAT_R16G16B16A16_UINT, 4}, {VK_FORMAT_R16G16B16A16_SINT, 4}, {VK_FORMAT_R16G16B16A16_SFLOAT, 4},
        {VK_FORMAT_R32G32B32A32_UINT, 4}, {VK_FORMAT_R32G32B32A32_SINT, 4}, {VK_FORMAT_R32G32B32A32_SFLOAT, 4},

        {VK_FORMAT_D16_UNORM, 1}, {VK_FORMAT_X8_D24_UNORM_PACK32, 1}, {VK_FORMAT_D32_SFLOAT, 1},
        {VK_FORMAT_S8_UINT, 1}, {VK_FORMAT_D24_UNORM_S8_UINT, 2}, {VK_FORMAT_D32_SFLOAT_S8_UINT, 2}
    };

    auto it = formatChannels.find(format);
    return (it != formatChannels.end()) ? it->second : 0; // Devuelve 0 si el formato es desconocido
}

CustomTexture::CustomTexture(unsigned int width, unsigned int height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, TEXTURE_TYPE type)
{
    ptrCommandPool = &commandPool;
    this->type = type;
    this->texHeight = height;
    this->texWidth = width;
    this->texChannels = this->GetChannelCount(format);

    VkImageUsageFlags usageFlag;

    if (type == TEXTURE_TYPE::COMPUTE_TYPE)
    {
        usageFlag = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    }
    else
    {
        usageFlag = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    }

    this->createImage(width, height, format, tiling, usageFlag, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    this->createTextureImageView(format);
    this->createTextureSampler();
}

CustomTexture::CustomTexture(std::string path, TEXTURE_TYPE type, QEColorSpace cs)
{
    this->type = type;
    this->texturePaths.push_back(path);

    if (type == TEXTURE_TYPE::CUBEMAP_TYPE)
    {
        this->createCubemapTextureImage(path);
    }
    else
    {
        std::filesystem::path p(path);
        std::string ext = p.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

        if (ext == ".ktx2")
            this->createTextureFromKtx2(path, cs);
        else
            this->createTextureImage(path, cs);
    }
}

void CustomTexture::createTextureImage(std::string path)
{
    createTextureImage(path, QEColorSpace::SRGB);
}

void CustomTexture::createTextureImage(std::string path, QEColorSpace cs)
{
    bool allocated = false;

    ptrCommandPool = &commandPool;
    stbi_uc* pixels = nullptr;

    if (path.empty())
    {
        this->texHeight = this->texWidth = 1;
        this->texChannels = 4;
        pixels = new stbi_uc[4]{ 0, 0, 0, 0 };
        allocated = true;
    }
    else
    {
        pixels = stbi_load(path.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    }

    if (!pixels)
        throw std::runtime_error("failed to load texture image!");

    mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;
    VkDeviceSize imageSize = static_cast<VkDeviceSize>(texWidth) * static_cast<VkDeviceSize>(texHeight) * 4;

    VkFormat imageFormat;
    if (path.empty())
        imageFormat = VK_FORMAT_R8G8B8A8_UNORM;
    else
        imageFormat = (cs == QEColorSpace::SRGB) ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM;

    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory stagingBufferMemory = VK_NULL_HANDLE;

    BufferManageModule::createBuffer(
        imageSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer,
        stagingBufferMemory,
        *deviceModule
    );

    void* data = nullptr;
    vkMapMemory(deviceModule->device, stagingBufferMemory, 0, imageSize, 0, &data);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(deviceModule->device, stagingBufferMemory);
  
    if (allocated) delete[] pixels;
    else stbi_image_free(pixels);

    createImage(
        texWidth,
        texHeight,
        imageFormat,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        mipLevels,
        1,
        VK_SAMPLE_COUNT_1_BIT
    );

    VkCommandBuffer cmd = beginSingleTimeCommands(deviceModule->device, *ptrCommandPool);

    VkImageSubresourceRange range{};
    range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    range.baseMipLevel = 0;
    range.levelCount = mipLevels;
    range.baseArrayLayer = 0;
    range.layerCount = 1;

    RecordTransitionImageLayout(
        cmd,
        image,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        range
    );

    RecordCopyBufferToImage(
        cmd,
        stagingBuffer,
        image,
        static_cast<uint32_t>(texWidth),
        static_cast<uint32_t>(texHeight)
    );

    RecordGenerateMipmaps(
        cmd,
        image,
        imageFormat,
        texWidth,
        texHeight,
        mipLevels
    );

    endSingleTimeCommands(deviceModule->device, queueModule->graphicsQueue, *ptrCommandPool, cmd);

    vkDestroyBuffer(deviceModule->device, stagingBuffer, nullptr);
    vkFreeMemory(deviceModule->device, stagingBufferMemory, nullptr);

    this->createTextureImageView(imageFormat);
    this->createTextureSampler();
}

KtxTranscodeSelection CustomTexture::ChooseKtxTranscodeFormat(QEColorSpace cs)
{
    VkPhysicalDeviceFeatures features{};
    vkGetPhysicalDeviceFeatures(deviceModule->physicalDevice, &features);

    if (features.textureCompressionBC)
    {
        if (cs == QEColorSpace::SRGB)
            return { KTX_TTF_BC7_RGBA, VK_FORMAT_BC7_SRGB_BLOCK };
        else
            return { KTX_TTF_BC7_RGBA, VK_FORMAT_BC7_UNORM_BLOCK };
    }

    if (cs == QEColorSpace::SRGB)
        return { KTX_TTF_RGBA32, VK_FORMAT_R8G8B8A8_SRGB };
    else
        return { KTX_TTF_RGBA32, VK_FORMAT_R8G8B8A8_UNORM };
}

void CustomTexture::createTextureFromKtx2(const std::string& path, QEColorSpace cs)
{
    ptrCommandPool = &commandPool;

    ktxTexture2* kTexture = nullptr;
    KTX_error_code ktxResult = ktxTexture2_CreateFromNamedFile(
        path.c_str(),
        KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT,
        &kTexture
    );

    if (ktxResult != KTX_SUCCESS || !kTexture)
        throw std::runtime_error("Failed to load KTX2 texture: " + path);

    VkFormat imageFormat = VK_FORMAT_UNDEFINED;

    if (ktxTexture2_NeedsTranscoding(kTexture))
    {
        KtxTranscodeSelection selection = ChooseKtxTranscodeFormat(cs);

        KTX_error_code transcodeResult = ktxTexture2_TranscodeBasis(
            kTexture,
            selection.ktxFormat,
            0
        );

        if (transcodeResult != KTX_SUCCESS)
        {
            ktxTexture_Destroy(ktxTexture(kTexture));
            throw std::runtime_error("Failed to transcode KTX2 texture: " + path);
        }

        imageFormat = selection.vkFormat;
    }
    else
    {
        imageFormat = static_cast<VkFormat>(kTexture->vkFormat);
    }

    if (imageFormat == VK_FORMAT_UNDEFINED)
    {
        ktxTexture_Destroy(ktxTexture(kTexture));
        throw std::runtime_error("KTX2 texture has undefined VkFormat: " + path);
    }

    texWidth = static_cast<int>(kTexture->baseWidth);
    texHeight = static_cast<int>(kTexture->baseHeight);
    mipLevels = kTexture->numLevels;
    texChannels = 4;

    ktx_size_t dataSize = ktxTexture_GetDataSize(ktxTexture(kTexture));
    ktx_uint8_t* imageData = ktxTexture_GetData(ktxTexture(kTexture));

    if (!imageData || dataSize == 0)
    {
        ktxTexture_Destroy(ktxTexture(kTexture));
        throw std::runtime_error("KTX2 texture contains no image data: " + path);
    }

    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory stagingBufferMemory = VK_NULL_HANDLE;

    BufferManageModule::createBuffer(
        static_cast<VkDeviceSize>(dataSize),
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer,
        stagingBufferMemory,
        *deviceModule
    );

    void* mapped = nullptr;
    vkMapMemory(deviceModule->device, stagingBufferMemory, 0, static_cast<VkDeviceSize>(dataSize), 0, &mapped);
    memcpy(mapped, imageData, static_cast<size_t>(dataSize));
    vkUnmapMemory(deviceModule->device, stagingBufferMemory);

    createImage(
        texWidth,
        texHeight,
        imageFormat,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        mipLevels,
        1,
        VK_SAMPLE_COUNT_1_BIT
    );

    VkCommandBuffer cmd = beginSingleTimeCommands(deviceModule->device, *ptrCommandPool);

    VkImageSubresourceRange range{};
    range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    range.baseMipLevel = 0;
    range.levelCount = mipLevels;
    range.baseArrayLayer = 0;
    range.layerCount = 1;

    RecordTransitionImageLayout(
        cmd,
        image,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        range
    );

    std::vector<VkBufferImageCopy> regions;
    regions.reserve(mipLevels);

    for (uint32_t level = 0; level < mipLevels; ++level)
    {
        ktx_size_t offset = 0;
        KTX_error_code offRes = ktxTexture_GetImageOffset(ktxTexture(kTexture), level, 0, 0, &offset);
        if (offRes != KTX_SUCCESS)
        {
            endSingleTimeCommands(deviceModule->device, queueModule->graphicsQueue, *ptrCommandPool, cmd);
            vkDestroyBuffer(deviceModule->device, stagingBuffer, nullptr);
            vkFreeMemory(deviceModule->device, stagingBufferMemory, nullptr);
            ktxTexture_Destroy(ktxTexture(kTexture));
            throw std::runtime_error("Failed to get KTX2 mip offset: " + path);
        }

        VkBufferImageCopy region{};
        region.bufferOffset = static_cast<VkDeviceSize>(offset);
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = level;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = { 0, 0, 0 };
        region.imageExtent = {
            std::max(1u, kTexture->baseWidth >> level),
            std::max(1u, kTexture->baseHeight >> level),
            1
        };

        regions.push_back(region);
    }

    vkCmdCopyBufferToImage(
        cmd,
        stagingBuffer,
        image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        static_cast<uint32_t>(regions.size()),
        regions.data()
    );

    RecordTransitionImageLayout(
        cmd,
        image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        range
    );

    endSingleTimeCommands(deviceModule->device, queueModule->graphicsQueue, *ptrCommandPool, cmd);

    vkDestroyBuffer(deviceModule->device, stagingBuffer, nullptr);
    vkFreeMemory(deviceModule->device, stagingBufferMemory, nullptr);

    ktxTexture_Destroy(ktxTexture(kTexture));

    createTextureImageView(imageFormat);
    createTextureSampler();
}

void CustomTexture::createCubemapTextureImage(std::vector<std::string> paths)
{
    ptrCommandPool = &commandPool;

    std::vector<stbi_uc*> pixels;

    for (int i = 0; i < 6; ++i)
    {
        if (paths[i].empty())
        {
            this->texHeight = this->texWidth = 1;
            this->texChannels = 4;
            pixels.push_back(new stbi_uc[4]{ 0, 0, 0, 0 });
        }
        else
        {
            pixels.push_back(stbi_load(paths[i].c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha));
        }

        if (!pixels.back())
            throw std::runtime_error("failed to load texture image!");
    }

    VkFormat imageFormat = stbi_is_hdr(paths[0].c_str())
        ? VK_FORMAT_R32G32B32A32_SFLOAT
        : VK_FORMAT_R8G8B8A8_SRGB;

    mipLevels = 1;

    VkDeviceSize stagingBufferSize = static_cast<VkDeviceSize>(texWidth) * static_cast<VkDeviceSize>(texHeight) * 4 * 6;
    VkDeviceSize imageSize = static_cast<VkDeviceSize>(texWidth) * static_cast<VkDeviceSize>(texHeight) * 4;

    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory stagingBufferMemory = VK_NULL_HANDLE;

    BufferManageModule::createBuffer(
        stagingBufferSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer,
        stagingBufferMemory,
        *deviceModule
    );

    VkDeviceSize offset = 0;
    for (int i = 0; i < 6; ++i)
    {
        void* data = nullptr;
        vkMapMemory(deviceModule->device, stagingBufferMemory, offset, imageSize, 0, &data);
        memcpy(data, pixels[i], static_cast<size_t>(imageSize));
        vkUnmapMemory(deviceModule->device, stagingBufferMemory);

        if (paths[i].empty()) delete[] pixels[i];
        else stbi_image_free(pixels[i]);

        offset += imageSize;
    }

    createImage(
        texWidth,
        texHeight,
        imageFormat,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        mipLevels,
        6,
        VK_SAMPLE_COUNT_1_BIT
    );

    VkCommandBuffer cmd = beginSingleTimeCommands(deviceModule->device, *ptrCommandPool);

    VkImageSubresourceRange range{};
    range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    range.baseMipLevel = 0;
    range.levelCount = mipLevels;
    range.baseArrayLayer = 0;
    range.layerCount = 6;

    RecordTransitionImageLayout(
        cmd,
        image,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        range
    );

    RecordCopyBufferImagesToCubemapImage(
        cmd,
        stagingBuffer,
        image,
        static_cast<uint32_t>(texWidth),
        imageSize,
        mipLevels
    );

    RecordTransitionImageLayout(
        cmd,
        image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        range
    );

    endSingleTimeCommands(deviceModule->device, queueModule->graphicsQueue, *ptrCommandPool, cmd);

    vkDestroyBuffer(deviceModule->device, stagingBuffer, nullptr);
    vkFreeMemory(deviceModule->device, stagingBufferMemory, nullptr);

    this->createTextureImageView(imageFormat);
    this->createCubemapTextureSampler();
}

void CustomTexture::createCubemapTextureImage(std::string path)
{
    ptrCommandPool = &commandPool;
    stbi_uc* pixels = nullptr;

    if (path.empty())
    {
        this->texHeight = this->texWidth = 1;
        this->texChannels = 4;
        pixels = new stbi_uc[4]{ 0, 0, 0, 0 };
    }
    else
    {
        pixels = stbi_load(path.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    }

    if (!pixels)
        throw std::runtime_error("failed to load texture image!");

    VkFormat imageFormat = stbi_is_hdr(path.c_str())
        ? VK_FORMAT_R32G32B32A32_SFLOAT
        : VK_FORMAT_R8G8B8A8_SRGB;

    uint32_t cubemapSize = static_cast<uint32_t>(texWidth) / 4;
    mipLevels = 1;

    VkDeviceSize imageSize = static_cast<VkDeviceSize>(texWidth) * static_cast<VkDeviceSize>(texHeight) * 4;
    VkDeviceSize requiredBufferSize = 6 * imageSize;

    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory stagingBufferMemory = VK_NULL_HANDLE;

    BufferManageModule::createBuffer(
        requiredBufferSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer,
        stagingBufferMemory,
        *deviceModule
    );

    void* data = nullptr;
    vkMapMemory(deviceModule->device, stagingBufferMemory, 0, requiredBufferSize, 0, &data);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(deviceModule->device, stagingBufferMemory);

    if (path.empty()) delete[] pixels;
    else stbi_image_free(pixels);

    createImage(
        cubemapSize,
        cubemapSize,
        imageFormat,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        mipLevels,
        6,
        VK_SAMPLE_COUNT_1_BIT
    );

    VkCommandBuffer cmd = beginSingleTimeCommands(deviceModule->device, *ptrCommandPool);

    VkImageSubresourceRange range{};
    range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    range.baseMipLevel = 0;
    range.levelCount = mipLevels;
    range.baseArrayLayer = 0;
    range.layerCount = 6;

    RecordTransitionImageLayout(
        cmd,
        image,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        range
    );

    RecordCopyBufferToCubemapImage(
        cmd,
        stagingBuffer,
        image,
        static_cast<uint32_t>(texWidth),
        static_cast<uint32_t>(texHeight),
        mipLevels
    );

    RecordTransitionImageLayout(
        cmd,
        image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        range
    );

    endSingleTimeCommands(deviceModule->device, queueModule->graphicsQueue, *ptrCommandPool, cmd);

    vkDestroyBuffer(deviceModule->device, stagingBuffer, nullptr);
    vkFreeMemory(deviceModule->device, stagingBufferMemory, nullptr);

    this->createTextureImageView(imageFormat);
    this->createCubemapTextureSampler();
}

void CustomTexture::createTextureRawImage(aiTexel* rawData, unsigned int width, unsigned int height)
{
    ptrCommandPool = &commandPool;

    this->texHeight = (height > 0) ? static_cast<int>(height) : 1;
    this->texWidth = (width > 0) ? static_cast<int>(width) : 1;
    this->texChannels = 4;

    mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;
    VkDeviceSize imageSize = static_cast<VkDeviceSize>(texWidth) * static_cast<VkDeviceSize>(texHeight) * 4;

    if (!rawData)
        throw std::runtime_error("failed to load texture image!");

    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory stagingBufferMemory = VK_NULL_HANDLE;

    BufferManageModule::createBuffer(
        imageSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer,
        stagingBufferMemory,
        *deviceModule
    );

    void* data = nullptr;
    vkMapMemory(deviceModule->device, stagingBufferMemory, 0, imageSize, 0, &data);
    memcpy(data, rawData, static_cast<size_t>(imageSize));
    vkUnmapMemory(deviceModule->device, stagingBufferMemory);

    stbi_image_free(rawData);

    uint32_t arrayLayers = (this->type == TEXTURE_TYPE::CUBEMAP_TYPE) ? 6u : 1u;

    createImage(
        texWidth,
        texHeight,
        VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        mipLevels,
        arrayLayers,
        VK_SAMPLE_COUNT_1_BIT
    );

    VkCommandBuffer cmd = beginSingleTimeCommands(deviceModule->device, *ptrCommandPool);

    VkImageSubresourceRange range{};
    range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    range.baseMipLevel = 0;
    range.levelCount = mipLevels;
    range.baseArrayLayer = 0;
    range.layerCount = arrayLayers;

    RecordTransitionImageLayout(
        cmd,
        image,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        range
    );

    RecordCopyBufferToImage(
        cmd,
        stagingBuffer,
        image,
        static_cast<uint32_t>(texWidth),
        static_cast<uint32_t>(texHeight)
    );

    RecordGenerateMipmaps(
        cmd,
        image,
        VK_FORMAT_R8G8B8A8_SRGB,
        texWidth,
        texHeight,
        mipLevels
    );

    endSingleTimeCommands(deviceModule->device, queueModule->graphicsQueue, *ptrCommandPool, cmd);

    vkDestroyBuffer(deviceModule->device, stagingBuffer, nullptr);
    vkFreeMemory(deviceModule->device, stagingBufferMemory, nullptr);

    this->createTextureImageView(VK_FORMAT_R8G8B8A8_SRGB);
    this->createTextureSampler();
}

void CustomTexture::createTextureImageView(VkFormat imageFormat)
{
    VkImageViewType viewType = (this->type == TEXTURE_TYPE::CUBEMAP_TYPE) ? VK_IMAGE_VIEW_TYPE_CUBE : VK_IMAGE_VIEW_TYPE_2D;
    uint32_t layerCount = (this->type == TEXTURE_TYPE::CUBEMAP_TYPE) ? 6 : 1;
    imageView = IMT::createImageView(deviceModule->device, image, viewType, imageFormat, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels, layerCount);
}

void CustomTexture::createTextureSampler()
{
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_FALSE; //VK_TRUE

    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(deviceModule->physicalDevice, &properties);
    samplerInfo.maxAnisotropy = 1;// properties.limits.maxSamplerAnisotropy;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.minLod = 0;
    samplerInfo.maxLod = static_cast<float>(mipLevels);
    samplerInfo.mipLodBias = 0.0f; // Optional

    if (vkCreateSampler(deviceModule->device, &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture sampler!");
    }
}

void CustomTexture::createCubemapTextureSampler()
{
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(deviceModule->physicalDevice, &properties);
    samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    samplerInfo.compareOp = VK_COMPARE_OP_NEVER;

    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.minLod = 0;
    samplerInfo.maxLod = static_cast<float>(mipLevels);
    samplerInfo.mipLodBias = 0.0f; // Optional

    if (vkCreateSampler(deviceModule->device, &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture sampler!");
    }
}

void CustomTexture::cleanup()
{
    if (textureSampler != VK_NULL_HANDLE)
    {
        vkDestroySampler(deviceModule->device, textureSampler, nullptr);
        textureSampler = VK_NULL_HANDLE;
    }

    if (imageView != VK_NULL_HANDLE)
    {
        vkDestroyImageView(deviceModule->device, imageView, nullptr);
        imageView = VK_NULL_HANDLE;
    }

    if (image != VK_NULL_HANDLE)
    {
        vkDestroyImage(deviceModule->device, image, nullptr);
        image = VK_NULL_HANDLE;
    }

    if (deviceMemory != VK_NULL_HANDLE)
    {
        vkFreeMemory(deviceModule->device, deviceMemory, nullptr);
        deviceMemory = VK_NULL_HANDLE;
    }
}

void CustomTexture::SaveTexturePath(std::ofstream& file)
{
    for (int i = 0; i < texturePaths.size(); i++)
    {
        std::string texturePath = texturePaths.at(i);
        int texturePathLength = static_cast<int>(texturePath.length());
        if (texturePathLength > 0)
        {
            file.write(reinterpret_cast<const char*>(&texturePathLength), sizeof(int));
            file.write(reinterpret_cast<const char*>(texturePath.c_str()), texturePathLength);
        }
    }
}

void CustomTexture::RecordTransitionImageLayout(
    VkCommandBuffer cmd,
    VkImage targetImage,
    VkImageLayout oldLayout,
    VkImageLayout newLayout,
    VkImageSubresourceRange range)
{
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = targetImage;
    barrier.subresourceRange = range;
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = 0;

    VkPipelineStageFlags sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    VkPipelineStageFlags destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
        newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
        newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
        newLayout == VK_IMAGE_LAYOUT_GENERAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_GENERAL &&
        newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
        newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask =
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL &&
        newLayout == VK_IMAGE_LAYOUT_GENERAL)
    {
        barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        destinationStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
    }
    else
    {
        throw std::invalid_argument("unsupported layout transition!");
    }

    vkCmdPipelineBarrier(
        cmd,
        sourceStage,
        destinationStage,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );

    this->currentLayout = newLayout;
}

void CustomTexture::RecordCopyBufferToImage(
    VkCommandBuffer cmd,
    VkBuffer buffer,
    VkImage targetImage,
    uint32_t width,
    uint32_t height)
{
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
        cmd,
        buffer,
        targetImage,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &region
    );
}

void CustomTexture::RecordCopyBufferToCubemapImage(
    VkCommandBuffer cmd,
    VkBuffer buffer,
    VkImage targetImage,
    uint32_t width,
    uint32_t height,
    uint32_t mipLevels)
{
    std::vector<VkBufferImageCopy> bufferCopyRegions;

    uint32_t cubemapSize = width / 4;
    uint32_t rowPitch = width * 4;

    VkOffset3D offsets[6] = {
        { static_cast<int32_t>(2 * cubemapSize), static_cast<int32_t>(cubemapSize), 0 },
        { 0, static_cast<int32_t>(cubemapSize), 0 },
        { static_cast<int32_t>(cubemapSize), 0, 0 },
        { static_cast<int32_t>(cubemapSize), static_cast<int32_t>(2 * cubemapSize), 0 },
        { static_cast<int32_t>(cubemapSize), static_cast<int32_t>(cubemapSize), 0 },
        { static_cast<int32_t>(3 * cubemapSize), static_cast<int32_t>(cubemapSize), 0 }
    };

    for (uint32_t face = 0; face < 6; ++face)
    {
        for (uint32_t level = 0; level < mipLevels; ++level)
        {
            VkBufferImageCopy region{};
            region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.imageSubresource.mipLevel = level;
            region.imageSubresource.baseArrayLayer = face;
            region.imageSubresource.layerCount = 1;

            region.imageExtent.width = cubemapSize >> level;
            region.imageExtent.height = cubemapSize >> level;
            region.imageExtent.depth = 1;

            region.bufferOffset = (offsets[face].y * rowPitch) + (offsets[face].x * 4);
            region.bufferRowLength = width;
            region.bufferImageHeight = height;

            bufferCopyRegions.push_back(region);
        }
    }

    vkCmdCopyBufferToImage(
        cmd,
        buffer,
        targetImage,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        static_cast<uint32_t>(bufferCopyRegions.size()),
        bufferCopyRegions.data()
    );
}

void CustomTexture::RecordCopyBufferImagesToCubemapImage(
    VkCommandBuffer cmd,
    VkBuffer buffer,
    VkImage targetImage,
    uint32_t textureWidth,
    VkDeviceSize faceTextureSize,
    uint32_t mipLevels)
{
    std::vector<VkBufferImageCopy> bufferCopyRegions;

    VkDeviceSize offset = 0;
    for (uint32_t face = 0; face < 6; ++face)
    {
        for (uint32_t level = 0; level < mipLevels; ++level)
        {
            VkBufferImageCopy region{};
            region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.imageSubresource.mipLevel = level;
            region.imageSubresource.baseArrayLayer = face;
            region.imageSubresource.layerCount = 1;

            region.imageExtent.width = textureWidth >> level;
            region.imageExtent.height = textureWidth >> level;
            region.imageExtent.depth = 1;

            region.bufferOffset = offset;
            region.bufferRowLength = 0;
            region.bufferImageHeight = 0;

            bufferCopyRegions.push_back(region);
        }

        offset += faceTextureSize;
    }

    vkCmdCopyBufferToImage(
        cmd,
        buffer,
        targetImage,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        static_cast<uint32_t>(bufferCopyRegions.size()),
        bufferCopyRegions.data()
    );
}

void CustomTexture::RecordGenerateMipmaps(
    VkCommandBuffer cmd,
    VkImage targetImage,
    VkFormat imageFormat,
    int32_t nTexWidth,
    int32_t nTexHeight,
    uint32_t nMipLevels)
{
    VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(deviceModule->physicalDevice, imageFormat, &formatProperties);

    if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
    {
        throw std::runtime_error("texture image format does not support linear blitting!");
    }

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.image = targetImage;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.levelCount = 1;

    int32_t mipWidth = nTexWidth;
    int32_t mipHeight = nTexHeight;

    for (uint32_t i = 1; i < nMipLevels; ++i)
    {
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier(
            cmd,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
        );

        VkImageBlit blit{};
        blit.srcOffsets[0] = { 0, 0, 0 };
        blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
        blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.mipLevel = i - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = 1;

        blit.dstOffsets[0] = { 0, 0, 0 };
        blit.dstOffsets[1] = {
            mipWidth > 1 ? mipWidth / 2 : 1,
            mipHeight > 1 ? mipHeight / 2 : 1,
            1
        };
        blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.mipLevel = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = 1;

        vkCmdBlitImage(
            cmd,
            targetImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            targetImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &blit,
            VK_FILTER_LINEAR
        );

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(
            cmd,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
        );

        if (mipWidth > 1) mipWidth /= 2;
        if (mipHeight > 1) mipHeight /= 2;
    }

    barrier.subresourceRange.baseMipLevel = nMipLevels - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(
        cmd,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );

    this->currentLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
}
