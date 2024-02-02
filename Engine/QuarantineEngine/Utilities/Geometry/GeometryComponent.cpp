#include "GeometryComponent.h"
#include "BufferManageModule.h"

DeviceModule* GeometryComponent::deviceModule_ptr;

void GeometryComponent::cleanup()
{
    if (this->indexBufferMemory != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(deviceModule_ptr->device, indexBuffer, nullptr);
        vkFreeMemory(deviceModule_ptr->device, indexBufferMemory, nullptr);
    }

    if (this->vertexBufferMemory != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(deviceModule_ptr->device, vertexBuffer, nullptr);
        vkFreeMemory(deviceModule_ptr->device, vertexBufferMemory, nullptr);
    }
}

void GeometryComponent::createIndexBuffer()
{
    VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    BufferManageModule::createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory, *deviceModule_ptr);

    void* data;
    vkMapMemory(deviceModule_ptr->device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, indices.data(), (size_t)bufferSize);
    vkUnmapMemory(deviceModule_ptr->device, stagingBufferMemory);

    //Rasterization -> VK_BUFFER_USAGE_INDEX_BUFFER_BIT
    //RayTracing -> VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
    BufferManageModule::createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory, *deviceModule_ptr);

    BufferManageModule::copyBuffer(stagingBuffer, indexBuffer, bufferSize, *deviceModule_ptr);

    vkDestroyBuffer(deviceModule_ptr->device, stagingBuffer, nullptr);
    vkFreeMemory(deviceModule_ptr->device, stagingBufferMemory, nullptr);
}

void GeometryComponent::createComputeBuffer()
{

}

VkVertexInputBindingDescription GeometryComponent::getBindingDescription(bool hasAnimation)
{
    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(PBRVertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return bindingDescription;
}

std::vector<VkVertexInputAttributeDescription> GeometryComponent::getAttributeDescriptions(bool hasAnimation) {
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};

    size_t numAttributes = (hasAnimation) ? 7 : 5;
    attributeDescriptions.resize(numAttributes);

    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(PBRVertex, pos);

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(PBRVertex, norm);

    attributeDescriptions[2].binding = 0;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[2].offset = offsetof(PBRVertex, texCoord);

    attributeDescriptions[3].binding = 0;
    attributeDescriptions[3].location = 3;
    attributeDescriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[3].offset = offsetof(PBRVertex, Tangents);

    return attributeDescriptions;
}
