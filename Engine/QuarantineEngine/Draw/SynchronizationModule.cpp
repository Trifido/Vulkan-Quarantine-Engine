#include "SynchronizationModule.h"
#include <stdexcept>

size_t SynchronizationModule::currentFrame = 0;

void SynchronizationModule::createSyncObjects(uint32_t swapChainImagesNum)
{
    imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    computeInFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
    computeFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        if (vkCreateSemaphore(deviceModule->device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(deviceModule->device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(deviceModule->device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create semaphores!");
        }

        if (vkCreateSemaphore(deviceModule->device, &semaphoreInfo, nullptr, &computeFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(deviceModule->device, &fenceInfo, nullptr, &computeInFlightFences[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create compute synchronization objects for a frame!");
        }
    }
}

void SynchronizationModule::cleanup()
{
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        vkDestroySemaphore(deviceModule->device, renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(deviceModule->device, imageAvailableSemaphores[i], nullptr);
        vkDestroyFence(deviceModule->device, inFlightFences[i], nullptr);
    }
}

void SynchronizationModule::submitCommandBuffer(VkCommandBuffer& commandBuffer)
{
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    //waitSemaphores = &imageAvailableSemaphores[currentFrame];
    VkSemaphore waitSemaphores[] = {computeFinishedSemaphores[currentFrame], imageAvailableSemaphores[currentFrame]};
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 2;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    signalSemaphores = &renderFinishedSemaphores[currentFrame];
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(queueModule->graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit draw command buffer!");
    }
}

void SynchronizationModule::submitComputeCommandBuffer(VkCommandBuffer& commandBuffer)
{
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &computeFinishedSemaphores[currentFrame];

    if (vkQueueSubmit(queueModule->computeQueue, 1, &submitInfo, computeInFlightFences[currentFrame]) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit compute command buffer!");
    };
}

VkResult SynchronizationModule::presentSwapchain(VkSwapchainKHR& swapChain, const uint32_t& imageIdx)
{
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = { swapChain };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIdx;
    presentInfo.pResults = nullptr;

    VkResult result = vkQueuePresentKHR(queueModule->presentQueue, &presentInfo);

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

    return result;
}

SynchronizationModule::SynchronizationModule()
{
    deviceModule = DeviceModule::getInstance();
    queueModule = QueueModule::getInstance();
}

void SynchronizationModule::synchronizeWaitFences()
{
    vkWaitForFences(deviceModule->device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
    vkResetFences(deviceModule->device, 1, &inFlightFences[currentFrame]);
}

void SynchronizationModule::synchronizeWaitComputeFences()
{
    vkWaitForFences(deviceModule->device, 1, &computeInFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
    vkResetFences(deviceModule->device, 1, &computeInFlightFences[currentFrame]);
}

size_t SynchronizationModule::GetCurrentFrame()
{
    return currentFrame;
}
