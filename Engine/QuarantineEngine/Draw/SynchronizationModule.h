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
    DeviceModule* deviceModule;
    size_t currentFrame = 0;
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    std::vector<VkFence> imagesInFlight;
    VkSemaphore* waitSemaphores;
    VkSemaphore* signalSemaphores;

public:
    SynchronizationModule();
    VkSemaphore getImageAvailableSemaphore() { return imageAvailableSemaphores[currentFrame]; };
    void createSyncObjects(uint32_t swapChainImagesNum);
    void cleanup();
    void submitCommandBuffer(VkCommandBuffer& commandBuffer, QueueModule& queueModule);
    VkResult presentSwapchain(VkSwapchainKHR& swapChain, const uint32_t& imageIdx, QueueModule& queueModule);
    void synchronizeWaitFences();
    void synchronizeCurrentFrame(const uint32_t& imageIdx);
    
};

#endif
