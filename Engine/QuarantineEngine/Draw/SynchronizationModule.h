#pragma once
#ifndef SYNCHRONIZATION_MODULE_H
#define SYNCHRONIZATION_MODULE_H

#include <vulkan/vulkan.hpp>
#include <vector>
#include "QueueModule.h"
#include "DeviceModule.h"

const int MAX_FRAMES_IN_FLIGHT = 2;

class SynchronizationModule
{
public:
private:
    DeviceModule*               deviceModule;
    QueueModule*                queueModule;
    static size_t               currentFrame;
    std::vector<VkSemaphore>    imageAvailableSemaphores;
    std::vector<VkSemaphore>    renderFinishedSemaphores;
    std::vector<VkFence>        inFlightFences;
    VkSemaphore*                waitSemaphores;
    VkSemaphore*                signalSemaphores;

    std::vector<VkFence>        computeInFlightFences;
    std::vector<VkSemaphore>    computeFinishedSemaphores;

public:
    SynchronizationModule();
    VkSemaphore getImageAvailableSemaphore() { return imageAvailableSemaphores[currentFrame]; };
    void createSyncObjects(uint32_t swapChainImagesNum);
    void cleanup();
    void submitCommandBuffer(VkCommandBuffer& commandBuffer);
    void submitComputeCommandBuffer(VkCommandBuffer& commandBuffer);
    VkResult presentSwapchain(VkSwapchainKHR& swapChain, const uint32_t& imageIdx);
    void synchronizeWaitFences();
    void synchronizeWaitComputeFences();
    static size_t GetCurrentFrame();
};

#endif
