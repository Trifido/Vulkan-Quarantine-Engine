#pragma once

#ifndef DESCRIPTOR_MODULE_H
#define DESCRIPTOR_MODULE_H

#include <vulkan/vulkan.h>
#include "DeviceModule.h"
#include "BufferManageModule.h"
#include "TextureModule.h"

class BufferManageModule;
class TextureModule;

class DescriptorModule
{
private:
    std::vector<VkDescriptorSet>    descriptorSets;
    VkDescriptorPool                descriptorPool;
    VkDescriptorSetLayout           descriptorSetLayout;

    BufferManageModule*             bufferModule;
    TextureModule*                  textureModule;

    DeviceModule*                   deviceModule;
    size_t                          descriptorCount;
public:
    DescriptorModule();
    void                            createDescriptorSetLayout();
    VkDescriptorSetLayout&          getDescriptorSetLayout() { return descriptorSetLayout; }
    VkDescriptorSet*                getDescriptorSet(size_t id) { return &descriptorSets.at(id); }
    std::vector<VkDescriptorSet>    getDescriptorSet() { return descriptorSets; }
    void    createDescriptorPool(size_t numSwapchainImgs);
    void    createDescriptorSets();
    void    addPtrData(BufferManageModule* bufferManageModule, TextureModule* texModule);
    void    cleanup();
    void    cleanupDescriptorPool();
};

#endif
