#pragma once

#ifndef DESCRIPTOR_LAYOUT
#define DESCRIPTOR_LAYOUT

#include <vulkan/vulkan.hpp>
#include "DeviceModule.h"

class DescriptorLayout
{
private:
    DeviceModule*           deviceModule;
    uint32_t                numUBOs = 0;
    uint32_t                numBinding = 0;
public:
    VkDescriptorPool        descriptorPool;
    VkDescriptorSetLayout   descriptorSetLayout;

public:
    DescriptorLayout();
    void    CreateDescriptorSetLayout();
    void    CreateDescriptorPool();
};

#endif 


