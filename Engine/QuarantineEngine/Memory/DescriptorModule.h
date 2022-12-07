#pragma once

#ifndef DESCRIPTOR_MODULE_H
#define DESCRIPTOR_MODULE_H

#include <vulkan/vulkan.hpp>
#include "DeviceModule.h"
#include "BufferManageModule.h"
#include "CustomTexture.h"
#include "Transform.h"
#include "UBO.h"
#include <map>

class CustomTexture;

class DescriptorModule
{
private:
    std::vector<VkDescriptorSet>    descriptorSets;
    VkDescriptorPool                descriptorPool;

    std::shared_ptr <std::vector<std::shared_ptr<CustomTexture>>> textures;
    uint32_t    numUBOs = 0;
    uint32_t    numBinding = 0;
    bool        hasAnimationProperties = false;

public:
    static  DeviceModule*           deviceModule;
    static  uint32_t                NumSwapchainImages;
    VkDescriptorSetLayout           descriptorSetLayout;

    //UBO's
    std::shared_ptr<UniformBufferObject>    cameraUBO;
    std::shared_ptr<UniformBufferObject>    materialUBO;
    std::shared_ptr<UniformBufferObject>    lightUBO;
    std::shared_ptr<UniformBufferObject>    animationUBO;

    //UNIFORM's
    std::shared_ptr<CameraUniform>          cameraUniform;
    std::shared_ptr<MaterialUniform>        materialUniform;
    std::shared_ptr<LightManagerUniform>    lightUniform;
    std::shared_ptr<AnimationUniform>       animationUniform;

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
    void    InitializeAnimationProperties();
    void    Initialize(std::shared_ptr <std::vector<std::shared_ptr<CustomTexture>>> textures, std::shared_ptr <MaterialUniform> uniformMaterial);

    void    createUniformBuffers();
    void    updateUniforms(uint32_t currentImage);
    void    recreateUniformBuffer();
};

#endif
