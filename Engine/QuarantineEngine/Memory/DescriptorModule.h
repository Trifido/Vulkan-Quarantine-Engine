#pragma once

#ifndef DESCRIPTOR_MODULE_H
#define DESCRIPTOR_MODULE_H

#include <vulkan/vulkan.hpp>
#include "DeviceModule.h"
#include "BufferManageModule.h"
#include "Texture.h"

class BufferManageModule;
class Texture;

class DescriptorModule
{
private:
    std::vector<VkDescriptorSet>    descriptorSets;
    VkDescriptorPool                descriptorPool;
    VkDescriptorSetLayout           descriptorSetLayout;

    BufferManageModule*             bufferModule;
    std::unique_ptr<Texture>        ptrTexture;

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
    void    addPtrData(BufferManageModule* bufferManageModule, Texture& texModule);
    void    cleanup();
    void    cleanupDescriptorPool();
};

#endif
