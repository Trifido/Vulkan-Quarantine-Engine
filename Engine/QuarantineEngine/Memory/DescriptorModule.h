#pragma once

#ifndef DESCRIPTOR_MODULE_H
#define DESCRIPTOR_MODULE_H

#include <vulkan/vulkan.hpp>
#include "DeviceModule.h"
#include "BufferManageModule.h"
#include "Texture.h"
#include "Transform.h"

class Texture;

class DescriptorModule
{
private:
    size_t                          numSwapchainImages;
    std::vector<VkBuffer>           uniformBuffers;
    std::vector<VkDeviceMemory>     uniformBuffersMemory;

    std::vector<VkDescriptorSet>    descriptorSets;
    VkDescriptorPool                descriptorPool;

    std::shared_ptr<Texture>        ptrTexture;

    DeviceModule*                   deviceModule;
    size_t                          descriptorCount;

public:
    VkDescriptorSetLayout           descriptorSetLayout;

public:
    DescriptorModule() {}
    DescriptorModule(DeviceModule& deviceModule);

    VkDescriptorSet*                getDescriptorSet(size_t id) { return &descriptorSets.at(id); }
    std::vector<VkDescriptorSet>    getDescriptorSet()          { return descriptorSets; }

    void    createDescriptorSetLayout();
    void    createDescriptorPool(size_t numSwapchainImgs);
    void    createDescriptorSets();
    void    addPtrData(Texture& texModule);
    void    cleanup();
    void    cleanupDescriptorPool();
    void    cleanupDescriptorBuffer();
    void    init(uint32_t numSwapChain, Texture& texModule);

    void    createUniformBuffers(size_t numImagesSwapChain);
    void    updateUniformBuffer(/*uint32_t currentImage,*/ VkExtent2D extent, std::shared_ptr<Transform> transform, int num);
    void    recreateUniformBuffer(uint32_t numSwapChain);
};

#endif
