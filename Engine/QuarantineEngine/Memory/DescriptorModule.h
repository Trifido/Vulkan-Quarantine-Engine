#pragma once

#ifndef DESCRIPTOR_MODULE_H
#define DESCRIPTOR_MODULE_H

#include <vulkan/vulkan.hpp>
#include "DeviceModule.h"
#include "BufferManageModule.h"
#include "Texture.h"
#include "Transform.h"
#include <map>

class Texture;

class DescriptorModule
{
private:
    std::vector<VkBuffer>           uniformBuffers;
    std::vector<VkDeviceMemory>     uniformBuffersMemory;

    std::vector<VkDescriptorSet>    descriptorSets;
    VkDescriptorPool                descriptorPool;

    std::vector<std::shared_ptr<Texture>> textures;

public:
    static  DeviceModule*           deviceModule;
    static  uint32_t                NumSwapchainImages;
    VkDescriptorSetLayout           descriptorSetLayout;

public:
    DescriptorModule();

    VkDescriptorSet*                getDescriptorSet(size_t id) { return &descriptorSets.at(id); }
    std::vector<VkDescriptorSet>    getDescriptorSet()          { return descriptorSets; }

    void    createDescriptorSetLayout();
    void    createDescriptorPool();
    void    createDescriptorSets();
    void    cleanup();
    void    cleanupDescriptorPool();
    void    cleanupDescriptorBuffer();
    void    Initialize(std::shared_ptr <std::map<TEXTURE_TYPE, std::shared_ptr<Texture>>> textures);

    void    createUniformBuffers();
    void    updateUniformBuffer(/*uint32_t currentImage,*/ VkExtent2D extent, glm::mat4& VPMainCamera ,std::shared_ptr<Transform> transform);
    void    recreateUniformBuffer();

private:
    void    InitializeTextureOrder(std::shared_ptr <std::map<TEXTURE_TYPE, std::shared_ptr<Texture>>> textureMap);
    void    CheckTextures(std::shared_ptr <std::map<TEXTURE_TYPE, std::shared_ptr<Texture>>> textureMap, TEXTURE_TYPE type);
};

#endif
