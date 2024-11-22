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
    lightManager = LightManager::getInstance();
    renderPassModule = RenderPassModule::getInstance();

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


void CommandPoolModule::setCustomRenderPass(VkFramebuffer& framebuffer, uint32_t iCBuffer)
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
    renderPassInfo.renderPass = *(this->renderPassModule->renderPass);
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

void CommandPoolModule::setDirectionalShadowRenderPass(std::shared_ptr<VkRenderPass> renderPass, std::shared_ptr<Light> light, uint32_t iCBuffer)
{
    uint32_t size = 0;
    VkFramebuffer frameBuffer = {};
    VkPipeline pipeline = {};
    VkPipelineLayout pipelineLayout = {};
    std::shared_ptr<DescriptorBuffer> descriptorBuffer = nullptr;
    float depthBiasConstant = 0;
    float depthBiasSlope = 0;

    //auto dirLight = std::dynamic_pointer_cast<DirectionalLight, Light>(light);
    //auto spotLight = std::dynamic_pointer_cast<SpotLight, Light>(light);
    //auto pointLight = std::dynamic_pointer_cast<PointLight, Light>(light);

    //if (dirLight != nullptr)
    //{
    //    size = dirLight->shadowMappingPtr->TextureSize;
    //    frameBuffer = dirLight->shadowMappingPtr->shadowFrameBuffer;
    //    depthBiasConstant = dirLight->shadowMappingPtr->depthBiasConstant;
    //    depthBiasSlope = dirLight->shadowMappingPtr->depthBiasSlope;
    //    pipeline = dirLight->shadowMappingPtr->shadowPipelineModule->pipeline;
    //    pipelineLayout = dirLight->shadowMappingPtr->shadowPipelineModule->pipelineLayout;
    //    descriptorBuffer = dirLight->descriptorBuffer;
    //}
    //if (spotLight != nullptr)
    //{
    //    size = spotLight->shadowMappingPtr->TextureSize;
    //    frameBuffer = spotLight->shadowMappingPtr->shadowFrameBuffer;
    //    depthBiasConstant = spotLight->shadowMappingPtr->depthBiasConstant;
    //    depthBiasSlope = spotLight->shadowMappingPtr->depthBiasSlope;
    //    pipeline = spotLight->shadowMappingPtr->shadowPipelineModule->pipeline;
    //    pipelineLayout = spotLight->shadowMappingPtr->shadowPipelineModule->pipelineLayout;
    //    descriptorBuffer = spotLight->descriptorBuffer;
    //}

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
    renderPassInfo.renderPass = *renderPass;
    renderPassInfo.framebuffer = frameBuffer;
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
    vkCmdSetDepthBias(commandBuffers[iCBuffer], depthBiasConstant, 0.0f, depthBiasSlope);
    vkCmdSetDepthTestEnable(commandBuffers[iCBuffer], true);
    vkCmdSetDepthWriteEnable(commandBuffers[iCBuffer], true);
    vkCmdSetFrontFace(commandBuffers[iCBuffer], VK_FRONT_FACE_CLOCKWISE);

    vkCmdBindPipeline(commandBuffers[iCBuffer], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

    vkCmdBindDescriptorSets(commandBuffers[iCBuffer], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, descriptorBuffer->getDescriptorSet(iCBuffer), 0, nullptr);
    this->gameObjectManager->ShadowCommand(commandBuffers[iCBuffer], iCBuffer, pipelineLayout);

    vkCmdEndRenderPass(commandBuffers[iCBuffer]);
}

void CommandPoolModule::setOmniShadowRenderPass(std::shared_ptr<VkRenderPass> renderPass, uint32_t idPointlight, uint32_t iCBuffer)
{
    auto pointLight = this->lightManager->PointLights.at(idPointlight);

    uint32_t size = pointLight->shadowMappingResourcesPtr->TextureSize;
    auto depthBiasConstant = pointLight->shadowMappingResourcesPtr->DepthBiasConstant;
    auto depthBiasSlope = pointLight->shadowMappingResourcesPtr->DepthBiasSlope;

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

    vkCmdSetViewport(commandBuffers[iCBuffer], 0, 1, &viewport);
    vkCmdSetScissor(commandBuffers[iCBuffer], 0, 1, &scissor);

    vkCmdSetDepthBias(commandBuffers[iCBuffer], depthBiasConstant, 0.0f, depthBiasSlope);
    vkCmdSetDepthTestEnable(commandBuffers[iCBuffer], true);
    vkCmdSetDepthWriteEnable(commandBuffers[iCBuffer], true);
    vkCmdSetFrontFace(commandBuffers[iCBuffer], VK_FRONT_FACE_CLOCKWISE);

    for (uint32_t faceId = 0; faceId < 6; faceId++)
    {
        this->updateCubeMapFace(faceId, renderPass, idPointlight, commandBuffers[iCBuffer], iCBuffer);
    }
}

void CommandPoolModule::updateCubeMapFace(uint32_t faceIdx, std::shared_ptr<VkRenderPass> renderPass, uint32_t idPointlight, VkCommandBuffer commandBuffer, uint32_t iCBuffer)
{
    VkClearValue clearValues[2];
    clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 1.0f } };
    clearValues[1].depthStencil = { 1.0f, 0 };

    auto pointLight = this->lightManager->PointLights.at(idPointlight);
    uint32_t size = pointLight->shadowMappingResourcesPtr->TextureSize;

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = *renderPass;
    renderPassInfo.framebuffer = pointLight->shadowMappingResourcesPtr->CubemapFacesFrameBuffers[faceIdx];
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent.width = size;
    renderPassInfo.renderArea.extent.height = size;
    renderPassInfo.clearValueCount = 2;
    renderPassInfo.pClearValues = clearValues;

    glm::mat4 viewMatrix = glm::mat4(1.0f);
    switch (faceIdx)
    {
    case 0: // POSITIVE_X
        viewMatrix = glm::rotate(viewMatrix, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        viewMatrix = glm::rotate(viewMatrix, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        break;
    case 1:	// NEGATIVE_X
        viewMatrix = glm::rotate(viewMatrix, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        viewMatrix = glm::rotate(viewMatrix, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        break;
    case 2:	// POSITIVE_Y
        viewMatrix = glm::rotate(viewMatrix, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        break;
    case 3:	// NEGATIVE_Y
        viewMatrix = glm::rotate(viewMatrix, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        break;
    case 4:	// POSITIVE_Z
        viewMatrix = glm::rotate(viewMatrix, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        break;
    case 5:	// NEGATIVE_Z
        viewMatrix = glm::rotate(viewMatrix, glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        break;
    }

    auto pipeline = lightManager->OmniShadowPipelineModule->pipeline;
    auto pipelineLayout = lightManager->OmniShadowPipelineModule->pipelineLayout;

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    //vkCmdPushConstants(
    //    commandBuffer,
    //    pipelineLayout,
    //    VK_SHADER_STAGE_ALL,
    //    0,
    //    sizeof(glm::mat4),
    //    &viewMatrix);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &lightManager->PointShadowDescritors->offscreenDescriptorSets[iCBuffer][idPointlight], 0, NULL);

    this->gameObjectManager->OmniShadowCommand(commandBuffers[iCBuffer], iCBuffer, pipelineLayout, viewMatrix, pointLight->transform->Position);

    vkCmdEndRenderPass(commandBuffer);
}

void CommandPoolModule::Render(FramebufferModule* framebufferModule)
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

        //auto itDirlight = this->lightManager->DirLights.begin();
        //while (itDirlight != this->lightManager->DirLights.end())
        //{
        //    this->setDirectionalShadowRenderPass(this->renderPassModule->dirShadowMappingRenderPass, *itDirlight, i);
        //    itDirlight++;
        //}

        //auto itSpotlight = this->lightManager->SpotLights.begin();
        //while (itSpotlight != this->lightManager->SpotLights.end())
        //{
        //    this->setDirectionalShadowRenderPass(this->renderPassModule->dirShadowMappingRenderPass, *itSpotlight, i);
        //    itSpotlight++;
        //}

        for(uint32_t idPointLight = 0; idPointLight < this->lightManager->PointLights.size(); idPointLight++)
        {
            this->setOmniShadowRenderPass(this->renderPassModule->omniShadowMappingRenderPass, idPointLight, i);
        }

        this->setCustomRenderPass(framebufferModule->swapChainFramebuffers[swapchainModule->currentImage], i);

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
