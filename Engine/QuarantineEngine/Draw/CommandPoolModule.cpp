#include "CommandPoolModule.h"
#include "QueueFamiliesModule.h"

#include <stdexcept>

#include <backends/imgui_impl_vulkan.h>
#include <SynchronizationModule.h>

CommandPoolModule* CommandPoolModule::instance = nullptr;

CommandPoolModule::CommandPoolModule()
{
    deviceModule = DeviceModule::getInstance();
    swapchainModule = SwapChainModule::getInstance();

    editorManager = EditorObjectManager::getInstance();
    gameObjectManager = GameObjectManager::getInstance();
    computeNodeManager = ComputeNodeManager::getInstance();
    cullingSceneManager = CullingSceneManager::getInstance();
    shadowMappingModule = ShadowMappingModule::getInstance();

    this->ClearColor = glm::vec3(0.1f);
}

CommandPoolModule* CommandPoolModule::getInstance()
{
    if (instance == NULL)
        instance = new CommandPoolModule();

    return instance;
}

void CommandPoolModule::ResetInstance()
{
    delete instance;
    instance = nullptr;
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

    VkCommandPoolCreateInfo computePoolInfo = {};
    computePoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    computePoolInfo.queueFamilyIndex = queueFamilyIndices.computeFamily.value();
    computePoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // Optional

    if (vkCreateCommandPool(deviceModule->device, &computePoolInfo, nullptr, &computeCommandPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create compute command pool!");
    }
}

void CommandPoolModule::createCommandBuffers()
{
    commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    computeCommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = this->commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

    if (vkAllocateCommandBuffers(deviceModule->device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffers!");
    }

    VkCommandBufferAllocateInfo computeAllocInfo{};
    computeAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    computeAllocInfo.commandPool = this->computeCommandPool;
    computeAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    computeAllocInfo.commandBufferCount = (uint32_t)computeCommandBuffers.size();

    if (vkAllocateCommandBuffers(deviceModule->device, &computeAllocInfo, computeCommandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffers!");
    }
}

void CommandPoolModule::recreateCommandBuffers()
{
    commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = this->commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

    if (vkAllocateCommandBuffers(deviceModule->device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffers!");
    }
}


void CommandPoolModule::setDefaultRenderPass(VkRenderPass& renderPass, VkFramebuffer& framebuffer, uint32_t iCBuffer)
{
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = swapchainModule->swapChainExtent.width;
    viewport.height = swapchainModule->swapChainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = swapchainModule->swapChainExtent;

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = framebuffer;
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = swapchainModule->swapChainExtent;

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = { this->ClearColor.x, this->ClearColor.y, this->ClearColor.z, 1.0f };
    clearValues[1].depthStencil = { 1.0f, 0 };
    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(commandBuffers[iCBuffer], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdSetViewport(commandBuffers[iCBuffer], 0, 1, &viewport);
    vkCmdSetScissor(commandBuffers[iCBuffer], 0, 1, &scissor);

    this->gameObjectManager->DrawCommand(commandBuffers[iCBuffer], iCBuffer);
    this->editorManager->DrawCommnad(commandBuffers[iCBuffer], iCBuffer);
    this->cullingSceneManager->DrawDebug(commandBuffers[iCBuffer], iCBuffer);

    if (ImGui::GetDrawData() != nullptr)
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffers[iCBuffer]);

    vkCmdEndRenderPass(commandBuffers[iCBuffer]);
}

void CommandPoolModule::setShadowRenderPass(VkRenderPass& renderPass, VkFramebuffer& framebuffer, uint32_t iCBuffer)
{
    uint32_t size = shadowMappingModule->TextureSize;

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = size;
    viewport.height = size;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent.width = size;
    scissor.extent.height = size;

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = framebuffer;
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent.width = size;
    renderPassInfo.renderArea.extent.height = size;

    VkClearValue clearValues{};
    clearValues.depthStencil = { 1.0f, 0 };
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearValues;

    vkCmdBeginRenderPass(commandBuffers[iCBuffer], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdSetViewport(commandBuffers[iCBuffer], 0, 1, &viewport);
    vkCmdSetScissor(commandBuffers[iCBuffer], 0, 1, &scissor);
    vkCmdSetDepthBias( commandBuffers[iCBuffer], shadowMappingModule->depthBiasConstant, 0.0f, shadowMappingModule->depthBiasSlope);

    vkCmdBindPipeline(commandBuffers[iCBuffer], VK_PIPELINE_BIND_POINT_GRAPHICS, this->shadowMappingModule->shadowPipelineModule->pipeline);
    this->gameObjectManager->ShadowCommand(commandBuffers[iCBuffer], iCBuffer, shadowMappingModule->shadowPipelineModule);
    //
    //vkCmdBindDescriptorSets(drawCmdBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets.offscreen, 0, nullptr);
    //scenes[sceneIndex].draw(drawCmdBuffers[i]);

    vkCmdEndRenderPass(commandBuffers[iCBuffer]);
}

void CommandPoolModule::Render(FramebufferModule* framebufferModule, RenderPassModule* renderPassModule)
{
    for (uint32_t i = 0; i < commandBuffers.size(); i++) {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0; // Optional
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        beginInfo.pInheritanceInfo = nullptr; // Optional

        if (vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("failed to begin recording command buffer!");
        }

        this->setShadowRenderPass(renderPassModule->shadowMappingRenderPass, framebufferModule->shadowMapFramebuffer, i);

        this->setDefaultRenderPass(renderPassModule->renderPass, framebufferModule->swapChainFramebuffers[swapchainModule->currentImage], i);

        if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to record command buffer!");
        }
    }
}

void CommandPoolModule::recordComputeCommandBuffer(VkCommandBuffer commandBuffer)
{
    auto currentFrame = SynchronizationModule::GetCurrentFrame();
    vkResetCommandBuffer(computeCommandBuffers[currentFrame], 0);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording compute command buffer!");
    }

    computeNodeManager->RecordComputeNodes(commandBuffer, currentFrame);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to record compute command buffer!");
    }
}

void CommandPoolModule::cleanup()
{
    vkDestroyCommandPool(deviceModule->device, computeCommandPool, nullptr);
    vkDestroyCommandPool(deviceModule->device, commandPool, nullptr);
}

void CommandPoolModule::CleanLastResources()
{
    this->deviceModule = nullptr;
    this->swapchainModule = nullptr;
    this->editorManager = nullptr;
    this->gameObjectManager = nullptr;
    this->computeNodeManager = nullptr;
    this->cullingSceneManager = nullptr;
}
