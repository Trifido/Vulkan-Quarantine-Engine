#pragma once
#ifndef EDITOR_OBJECT_H
#define EDITOR_OBJECT_H

#include <vulkan/vulkan.hpp>

class EditorObject
{
public:
    virtual void Clean() = 0;
    virtual void Draw(VkCommandBuffer& commandBuffer, uint32_t idx) = 0;
};

#endif
