#include "CustomTexture.h"

#include <stb_image.h>

#include "BufferManageModule.h"
#include "SyncTool.h"
#include <unordered_map>


VkCommandPool CustomTexture::commandPool;

void CustomTexture::copyBufferToImage(VkBuffer buffer, VkImage nImage, uint32_t width, uint32_t height, uint32_t mipLevels)
{
    VkCommandBuffer commandBuffer = beginSingleTimeCommands(deviceModule->device, *ptrCommandPool);
    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;

    region.imageOffset = { 0, 0, 0 };
    region.imageExtent = {
        width,
        height,
        1
    };

    VkImageSubresourceRange range = {};
    range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    range.baseMipLevel = 0;
    range.levelCount = mipLevels;
    range.baseArrayLayer = 0;
    range.layerCount = 1;

    transitionImageLayout(image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, range);

    vkCmdCopyBufferToImage(
        commandBuffer,
        buffer,
        nImage,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &region
    );
    endSingleTimeCommands(deviceModule->device, queueModule->graphicsQueue, *ptrCommandPool, commandBuffer);
}

void CustomTexture::copyBufferToCubemapImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t mipLevels)
{
    VkCommandBuffer commandBuffer = beginSingleTimeCommands(deviceModule->device, *ptrCommandPool);
    std::vector<VkBufferImageCopy> bufferCopyRegions;

    uint32_t cubemapSize = width / 4; // Tamaño de cada cara (asume atlas cuadrado)
    uint32_t rowPitch = width * 4;   // Asume RGBA8, 4 bytes por píxel

    VkOffset3D offsets[6] = {
        {2 * cubemapSize, cubemapSize, 0},  // +X
        {0, cubemapSize, 0},               // -X
        {cubemapSize, 0, 0},               // +Y
        {cubemapSize, 2 * cubemapSize, 0}, // -Y
        {cubemapSize, cubemapSize, 0},     // +Z
        {3 * cubemapSize, cubemapSize, 0}  // -Z
    };

    for (uint32_t face = 0; face < 6; face++)
    {
        for (uint32_t level = 0; level < mipLevels; level++)
        {
            // Calculate offset into staging buffer for the current mip level and face
            VkBufferImageCopy bufferCopyRegion = {};
            bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            bufferCopyRegion.imageSubresource.mipLevel = level;
            bufferCopyRegion.imageSubresource.baseArrayLayer = face;
            bufferCopyRegion.imageSubresource.layerCount = 1;

            bufferCopyRegion.imageExtent.width = cubemapSize >> level;
            bufferCopyRegion.imageExtent.height = cubemapSize >> level;
            bufferCopyRegion.imageExtent.depth = 1;
            bufferCopyRegion.bufferOffset = (offsets[face].y * rowPitch) + (offsets[face].x * 4);
            bufferCopyRegion.bufferRowLength = width; // Anchura total del atlas
            bufferCopyRegion.bufferImageHeight = height; // Altura total del atlas

            bufferCopyRegions.push_back(bufferCopyRegion);
        }
    }

    VkImageSubresourceRange subresourceRange = {};
    subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresourceRange.baseMipLevel = 0;
    subresourceRange.levelCount = mipLevels;
    subresourceRange.layerCount = 6;

    transitionImageLayout(image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, subresourceRange);

    vkCmdCopyBufferToImage(
        commandBuffer,
        buffer,
        image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        static_cast<uint32_t>(bufferCopyRegions.size()),
        bufferCopyRegions.data()
    );

    endSingleTimeCommands(deviceModule->device, queueModule->graphicsQueue, *ptrCommandPool, commandBuffer);
}

void CustomTexture::copyBufferImagesToCubemapImage(VkBuffer buffer, VkImage image, uint32_t textureWidth, VkDeviceSize faceTextureSize, uint32_t mipLevels)
{
    VkCommandBuffer commandBuffer = beginSingleTimeCommands(deviceModule->device, *ptrCommandPool);
    std::vector<VkBufferImageCopy> bufferCopyRegions;;

    VkDeviceSize offset = 0;
    for (uint32_t face = 0; face < 6; face++)
    {
        for (uint32_t level = 0; level < mipLevels; level++)
        {
            // Calculate offset into staging buffer for the current mip level and face
            VkBufferImageCopy bufferCopyRegion = {};
            bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            bufferCopyRegion.imageSubresource.mipLevel = level;
            bufferCopyRegion.imageSubresource.baseArrayLayer = face;
            bufferCopyRegion.imageSubresource.layerCount = 1;

            bufferCopyRegion.imageExtent.width = textureWidth >> level;
            bufferCopyRegion.imageExtent.height = textureWidth >> level;
            bufferCopyRegion.imageExtent.depth = 1;
            bufferCopyRegion.bufferOffset = offset;
            bufferCopyRegion.bufferRowLength = 0; // width; // Anchura total del atlas
            bufferCopyRegion.bufferImageHeight = 0; // height; // Altura total del atlas

            bufferCopyRegions.push_back(bufferCopyRegion);
        }
        offset += faceTextureSize;
    }

    VkImageSubresourceRange subresourceRange = {};
    subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresourceRange.baseMipLevel = 0;
    subresourceRange.levelCount = mipLevels;
    subresourceRange.layerCount = 6;

    transitionImageLayout(image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, subresourceRange);

    vkCmdCopyBufferToImage(
        commandBuffer,
        buffer,
        image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        static_cast<uint32_t>(bufferCopyRegions.size()),
        bufferCopyRegions.data()
    );

    endSingleTimeCommands(deviceModule->device, queueModule->graphicsQueue, *ptrCommandPool, commandBuffer);
}

void CustomTexture::generateMipmaps(VkImage nImage, VkFormat imageFormat, int32_t nTexWidth, int32_t nTexHeight, uint32_t nMipLevels)
{
    // Check if image format supports linear blitting
    VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(deviceModule->physicalDevice, imageFormat, &formatProperties);

    if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
        throw std::runtime_error("texture image format does not support linear blitting!");
    }

    VkCommandBuffer commandBuffer = beginSingleTimeCommands(deviceModule->device, *ptrCommandPool);

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.image = nImage;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.levelCount = 1;

    int32_t mipWidth = nTexWidth;
    int32_t mipHeight = nTexHeight;

    for (uint32_t i = 1; i < mipLevels; i++)
    {
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
            0, nullptr,
            0, nullptr,
            1, &barrier);

        VkImageBlit blit{};
        blit.srcOffsets[0] = { 0, 0, 0 };
        blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
        blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.mipLevel = i - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = 1;
        blit.dstOffsets[0] = { 0, 0, 0 };
        blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
        blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.mipLevel = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = 1;

        vkCmdBlitImage(commandBuffer,
            image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &blit,
            VK_FILTER_LINEAR);

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
            0, nullptr,
            0, nullptr,
            1, &barrier);

        if (mipWidth > 1) mipWidth /= 2;
        if (mipHeight > 1) mipHeight /= 2;
    }

    barrier.subresourceRange.baseMipLevel = nMipLevels - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(commandBuffer,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
        0, nullptr,
        0, nullptr,
        1, &barrier);


    endSingleTimeCommands(deviceModule->device, queueModule->graphicsQueue, *ptrCommandPool, commandBuffer);
}

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

CustomTexture::CustomTexture(vector<string> path)
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

void CustomTexture::createTextureImage(std::string path)
{
    ptrCommandPool = &commandPool;
    stbi_uc* pixels;

    if (path.empty())
    {
        this->texHeight = this->texWidth = 1;
        this->texChannels = 3;
        pixels = new unsigned char(0);
    }
    else
    {
        pixels = stbi_load(path.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    }

    mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

    VkDeviceSize imageSize = texWidth * texHeight * 4;
    VkFormat imageFormat = stbi_is_hdr(path.c_str()) ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_SRGB;

    if (!pixels) {
        throw std::runtime_error("failed to load texture image!");
    }

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    BufferManageModule::createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory, *deviceModule);

    void* data;
    vkMapMemory(deviceModule->device, stagingBufferMemory, 0, imageSize, 0, &data);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(deviceModule->device, stagingBufferMemory);

    stbi_image_free(pixels);

    createImage(texWidth, texHeight, imageFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, mipLevels, 1, VK_SAMPLE_COUNT_1_BIT);

    copyBufferToImage(stagingBuffer, image, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), mipLevels);

    vkDestroyBuffer(deviceModule->device, stagingBuffer, nullptr);
    vkFreeMemory(deviceModule->device, stagingBufferMemory, nullptr);

    generateMipmaps(image, imageFormat, texWidth, texHeight, mipLevels);

    this->createTextureImageView(imageFormat);
    this->createTextureSampler();
}

void CustomTexture::createCubemapTextureImage(std::string path)
{
    ptrCommandPool = &commandPool;
    stbi_uc* pixels;

    if (path.empty())
    {
        this->texHeight = this->texWidth = 1;
        this->texChannels = 3;
        pixels = new unsigned char(0);
    }
    else
    {
        pixels = stbi_load(path.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    }

    VkFormat imageFormat = stbi_is_hdr(path.c_str()) ? VK_FORMAT_R32G32B32A32_SFLOAT : VK_FORMAT_R8G8B8A8_SRGB;
    uint32_t cubemapSize = texWidth / 4;
    mipLevels = 1;

    VkDeviceSize imageSize = texWidth * texHeight * 4;
    VkDeviceSize requiredBufferSize = 6 * imageSize;
    if (!pixels) {
        throw std::runtime_error("failed to load texture image!");
    }

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    BufferManageModule::createBuffer(requiredBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory, *deviceModule);

    void* data;
    vkMapMemory(deviceModule->device, stagingBufferMemory, 0, requiredBufferSize, 0, &data);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(deviceModule->device, stagingBufferMemory);

    stbi_image_free(pixels);

    createImage(cubemapSize, cubemapSize, imageFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, mipLevels, 6, VK_SAMPLE_COUNT_1_BIT);

    copyBufferToCubemapImage(stagingBuffer, image, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), mipLevels);

    //If not mipmap
    VkImageSubresourceRange subresourceRange = {};
    subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresourceRange.baseMipLevel = 0;
    subresourceRange.levelCount = mipLevels;
    subresourceRange.layerCount = 6;
    transitionImageLayout(image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, subresourceRange);

    vkDestroyBuffer(deviceModule->device, stagingBuffer, nullptr);
    vkFreeMemory(deviceModule->device, stagingBufferMemory, nullptr);

    this->createTextureImageView(imageFormat);
    this->createCubemapTextureSampler();
}

void CustomTexture::createCubemapTextureImage(vector<string> paths)
{
    ptrCommandPool = &commandPool;

    vector<stbi_uc*> pixels;

    for (int i = 0; i < 6; i++)
    {
        if (paths[i].empty())
        {
            this->texHeight = this->texWidth = 1;
            this->texChannels = 3;
            pixels.push_back(new unsigned char(0));
        }
        else
        {
            pixels.push_back(stbi_load(paths[i].c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha));
        }

        if (!pixels.back())
        {
            throw std::runtime_error("failed to load texture image!");
        }
    }
    
    VkFormat imageFormat = stbi_is_hdr(paths[0].c_str()) ? VK_FORMAT_R32G32B32A32_SFLOAT : VK_FORMAT_R8G8B8A8_SRGB;
    mipLevels = 1;

    VkDeviceSize stagingBufferSize = texWidth * texHeight * 4 * 6;
    VkDeviceSize imageSize = texWidth * texHeight * 4;
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    BufferManageModule::createBuffer(stagingBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory, *deviceModule);

    VkDeviceSize offset = 0;
    for (int i = 0; i < 6; i++)
    {
        void* data;
        vkMapMemory(deviceModule->device, stagingBufferMemory, offset, imageSize, 0, &data);
        memcpy(data, pixels[i], static_cast<size_t>(imageSize));
        vkUnmapMemory(deviceModule->device, stagingBufferMemory);

        stbi_image_free(pixels[i]);

        offset += imageSize;
    }

    createImage(texWidth, texHeight, imageFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, mipLevels, 6, VK_SAMPLE_COUNT_1_BIT);

    copyBufferImagesToCubemapImage(stagingBuffer, image, static_cast<uint32_t>(texWidth), imageSize, mipLevels);

    //If not mipmap
    VkImageSubresourceRange subresourceRange = {};
    subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresourceRange.baseMipLevel = 0;
    subresourceRange.levelCount = mipLevels;
    subresourceRange.layerCount = 6;
    transitionImageLayout(image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, subresourceRange);

    vkDestroyBuffer(deviceModule->device, stagingBuffer, nullptr);
    vkFreeMemory(deviceModule->device, stagingBufferMemory, nullptr);

    this->createTextureImageView(imageFormat);
    this->createCubemapTextureSampler();
}

void CustomTexture::createTextureRawImage(aiTexel* rawData, unsigned int width, unsigned int height)
{
    ptrCommandPool = &commandPool;
    //stbi_uc* pixels;

    this->texHeight = (height > 0) ? height : 1;
    this->texWidth = (width > 0) ? width : 1;
    this->texChannels = 4;
    //pixels = rawData-;

    mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

    VkDeviceSize imageSize = texWidth * texHeight * 4;

    if (!rawData) {
        throw std::runtime_error("failed to load texture image!");
    }

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    BufferManageModule::createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory, *deviceModule);

    void *data;
    vkMapMemory(deviceModule->device, stagingBufferMemory, 0, imageSize, 0, &data);
    memcpy(data, rawData, static_cast<size_t>(imageSize));
    vkUnmapMemory(deviceModule->device, stagingBufferMemory);

    stbi_image_free(rawData);

    uint32_t arrayLayers = (this->type == TEXTURE_TYPE::CUBEMAP_TYPE) ? 6 : 1;
    createImage(texWidth, texHeight, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, mipLevels, arrayLayers, VK_SAMPLE_COUNT_1_BIT);

    copyBufferToImage(stagingBuffer, image, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), mipLevels);

    vkDestroyBuffer(deviceModule->device, stagingBuffer, nullptr);
    vkFreeMemory(deviceModule->device, stagingBufferMemory, nullptr);

    generateMipmaps(image, VK_FORMAT_R8G8B8A8_SRGB, texWidth, texHeight, mipLevels);

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
        int texturePathLength = texturePath.length();
        if (texturePathLength > 0)
        {
            file.write(reinterpret_cast<const char*>(&texturePathLength), sizeof(int));
            file.write(reinterpret_cast<const char*>(texturePath.c_str()), texturePathLength);
        }
    }
}
