#include "CommandPoolModule.h"
#include "QueueFamiliesModule.h"

#include <stdexcept>

#include <backends/imgui_impl_vulkan.h>

CommandPoolModule* CommandPoolModule::instance = nullptr;

CommandPoolModule::CommandPoolModule()
{
    deviceModule = DeviceModule::getInstance();
}

CommandPoolModule* CommandPoolModule::getInstance()
{
    if (instance == NULL)
        instance = new CommandPoolModule();
    else
        std::cout << "Getting existing instance Command Pool Module" << std::endl;

    return instance;
}

void CommandPoolModule::createCommandPool(VkSurfaceKHR& surface)
{
    QueueFamilyIndices queueFamilyIndices = QueueFamilyIndices::findQueueFamilies(deviceModule->physicalDevice, surface);

    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // Optional

    if (vkCreateCommandPool(deviceModule->device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create command pool!");
    }
}

void CommandPoolModule::createCommandBuffers(std::vector<VkFramebuffer>& swapChainFramebuffers, VkRenderPass& renderPass,
    VkExtent2D& swapChainExtent, VkPipelineLayout& pipelineLayout, VkPipeline& pipeline, std::vector<std::shared_ptr<GameObject>>& gameObjects)
{
    commandBuffers.resize(swapChainFramebuffers.size());

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

    if (vkAllocateCommandBuffers(deviceModule->device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffers!");
    }

    recreateCommandBuffers(swapChainFramebuffers, renderPass, swapChainExtent, pipelineLayout, pipeline, gameObjects);
}

void CommandPoolModule::recreateCommandBuffers(std::vector<VkFramebuffer>& swapChainFramebuffers, VkRenderPass& renderPass,
    VkExtent2D& swapChainExtent, VkPipelineLayout& pipelineLayout, VkPipeline& pipeline, std::vector<std::shared_ptr<GameObject>>& gameObjects)
{
    for (size_t i = 0; i < commandBuffers.size(); i++) {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0; // Optional
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        beginInfo.pInheritanceInfo = nullptr; // Optional

        if (vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("failed to begin recording command buffer!");
        }

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPass;
        renderPassInfo.framebuffer = swapChainFramebuffers[i];
        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = swapChainExtent;

        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
        clearValues[1].depthStencil = { 1.0f, 0 };
        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        for (int j = 0; j < gameObjects.size(); j++)
        {
            gameObjects.at(j)->drawCommand(commandBuffers[i], pipelineLayout, pipeline, i);
        }

        if(ImGui::GetDrawData() != nullptr)
            ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffers[i]);

        vkCmdEndRenderPass(commandBuffers[i]);

        if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to record command buffer!");
        }
    }
}

void CommandPoolModule::cleanup()
{
    vkDestroyCommandPool(deviceModule->device, commandPool, nullptr);
}
