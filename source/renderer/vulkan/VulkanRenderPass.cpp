/*
* Copyright (c) 2006-2025, Ondrej Danek (www.ondrej-danek.net)
* All rights reserved.
* Redistribution and use in source and binary forms, with or without modification,
* are permitted provided that the following conditions are met:
*
*     * Redistributions of source code must retain the above copyright notice,
*       this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright notice,
*       this list of conditions and the following disclaimer in the documentation
*       and/or other materials provided with the distribution.
*     * Neither the name of Ondrej Danek nor the names of its contributors may be
*       used to endorse or promote products derived from this software without
*       specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
* IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
* BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
* DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
* OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
* EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "VulkanRenderPass.h"
#include "VulkanDevice.h"
#include "VulkanSwapchain.h"
#include "../../Exception.h"
#include "../../VideoException.h"
#include <array>
#include <iostream>

namespace Duel6 {

    // VulkanRenderPass implementation
    VulkanRenderPass::VulkanRenderPass(const VulkanDevice& device, const RenderPassConfig& config)
        : device(&device), renderPass(VK_NULL_HANDLE), config(config),
          depthImage(VK_NULL_HANDLE), depthImageView(VK_NULL_HANDLE), depthImageMemory(VK_NULL_HANDLE),
          width(0), height(0) {
        createRenderPass();
    }

    VulkanRenderPass::~VulkanRenderPass() {
        cleanup();
    }

    VulkanRenderPass::VulkanRenderPass(VulkanRenderPass&& other) noexcept
        : device(other.device), renderPass(other.renderPass), config(other.config),
          framebuffers(std::move(other.framebuffers)), colorImageViews(std::move(other.colorImageViews)),
          depthImage(other.depthImage), depthImageView(other.depthImageView), depthImageMemory(other.depthImageMemory),
          width(other.width), height(other.height) {
        other.device = nullptr;
        other.renderPass = VK_NULL_HANDLE;
        other.depthImage = VK_NULL_HANDLE;
        other.depthImageView = VK_NULL_HANDLE;
        other.depthImageMemory = VK_NULL_HANDLE;
    }

    VulkanRenderPass& VulkanRenderPass::operator=(VulkanRenderPass&& other) noexcept {
        if (this != &other) {
            cleanup();
            
            device = other.device;
            renderPass = other.renderPass;
            config = other.config;
            framebuffers = std::move(other.framebuffers);
            colorImageViews = std::move(other.colorImageViews);
            depthImage = other.depthImage;
            depthImageView = other.depthImageView;
            depthImageMemory = other.depthImageMemory;
            width = other.width;
            height = other.height;
            
            other.device = nullptr;
            other.renderPass = VK_NULL_HANDLE;
            other.depthImage = VK_NULL_HANDLE;
            other.depthImageView = VK_NULL_HANDLE;
            other.depthImageMemory = VK_NULL_HANDLE;
        }
        return *this;
    }

    void VulkanRenderPass::createFramebuffers(const VulkanSwapchain& swapchain) {
        width = swapchain.getExtent().width;
        height = swapchain.getExtent().height;
        
        // Clean up existing framebuffers
        cleanupFramebuffers();
        
        // Create depth resources if needed
        if (config.hasDepthBuffer) {
            createDepthResources();
        }
        
        // Get swapchain image views
        colorImageViews = swapchain.getImageViews();
        
        // Create framebuffers
        framebuffers.resize(colorImageViews.size());
        
        for (size_t i = 0; i < colorImageViews.size(); i++) {
            std::vector<VkImageView> attachments;
            
            // Color attachment
            attachments.push_back(colorImageViews[i]);
            
            // Depth attachment (if enabled)
            if (config.hasDepthBuffer) {
                attachments.push_back(depthImageView);
            }
            
            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = renderPass;
            framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
            framebufferInfo.pAttachments = attachments.data();
            framebufferInfo.width = width;
            framebufferInfo.height = height;
            framebufferInfo.layers = 1;
            
            VkResult result = vkCreateFramebuffer(device->getLogicalDevice(), &framebufferInfo, nullptr, &framebuffers[i]);
            if (result != VK_SUCCESS) {
                D6_THROW(VideoException, "Failed to create framebuffer: " + std::to_string(result));
            }
        }
    }

    void VulkanRenderPass::recreateFramebuffers(const VulkanSwapchain& swapchain) {
        createFramebuffers(swapchain);
    }

    void VulkanRenderPass::begin(VkCommandBuffer commandBuffer, uint32_t framebufferIndex, VkSubpassContents contents) const {
        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPass;
        renderPassInfo.framebuffer = framebuffers[framebufferIndex];
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = {width, height};
        
        std::vector<VkClearValue> clearValues;
        
        if (config.clearColor) {
            VkClearValue colorClear{};
            colorClear.color = config.clearColorValue;
            clearValues.push_back(colorClear);
        }
        
        if (config.hasDepthBuffer && config.clearDepth) {
            VkClearValue depthClear{};
            depthClear.depthStencil = {config.clearDepthValue, config.stencilClearValue};
            clearValues.push_back(depthClear);
        }
        
        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();
        
        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, contents);
    }

    void VulkanRenderPass::end(VkCommandBuffer commandBuffer) const {
        vkCmdEndRenderPass(commandBuffer);
    }

    VkFramebuffer VulkanRenderPass::getFramebuffer(uint32_t index) const {
        if (index >= framebuffers.size()) {
            D6_THROW(Exception, "Framebuffer index out of range");
        }
        return framebuffers[index];
    }

    void VulkanRenderPass::createRenderPass() {
        std::vector<VkAttachmentDescription> attachments;
        std::vector<VkAttachmentReference> colorAttachmentRefs;
        VkAttachmentReference depthAttachmentRef{};
        
        // Color attachment
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = config.colorFormat;
        colorAttachment.samples = config.samples;
        colorAttachment.loadOp = config.clearColor ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        
        attachments.push_back(colorAttachment);
        
        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorAttachmentRefs.push_back(colorAttachmentRef);
        
        // Depth attachment (if enabled)
        if (config.hasDepthBuffer) {
            VkAttachmentDescription depthAttachment{};
            depthAttachment.format = config.depthFormat;
            depthAttachment.samples = config.samples;
            depthAttachment.loadOp = config.clearDepth ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
            depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            
            attachments.push_back(depthAttachment);
            
            depthAttachmentRef.attachment = 1;
            depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        }
        
        // Subpass
        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = static_cast<uint32_t>(colorAttachmentRefs.size());
        subpass.pColorAttachments = colorAttachmentRefs.data();
        if (config.hasDepthBuffer) {
            subpass.pDepthStencilAttachment = &depthAttachmentRef;
        }
        
        // Subpass dependencies
        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        
        if (config.hasDepthBuffer) {
            dependency.srcStageMask |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
            dependency.dstStageMask |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
            dependency.dstAccessMask |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        }
        
        // Create render pass
        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        renderPassInfo.pAttachments = attachments.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;
        
        VkResult result = vkCreateRenderPass(device->getLogicalDevice(), &renderPassInfo, nullptr, &renderPass);
        if (result != VK_SUCCESS) {
            D6_THROW(VideoException, "Failed to create render pass: " + std::to_string(result));
        }
    }

    void VulkanRenderPass::createDepthResources() {
        // Find supported depth format
        VkFormat depthFormat = config.depthFormat;
        
        // Create depth image
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = width;
        imageInfo.extent.height = height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = depthFormat;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        imageInfo.samples = config.samples;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        
        VkResult result = vkCreateImage(device->getLogicalDevice(), &imageInfo, nullptr, &depthImage);
        if (result != VK_SUCCESS) {
            D6_THROW(VideoException, "Failed to create depth image: " + std::to_string(result));
        }
        
        // Allocate memory for depth image
        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(device->getLogicalDevice(), depthImage, &memRequirements);
        
        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = device->findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        
        result = vkAllocateMemory(device->getLogicalDevice(), &allocInfo, nullptr, &depthImageMemory);
        if (result != VK_SUCCESS) {
            D6_THROW(VideoException, "Failed to allocate depth image memory: " + std::to_string(result));
        }
        
        vkBindImageMemory(device->getLogicalDevice(), depthImage, depthImageMemory, 0);
        
        // Create depth image view
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = depthImage;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = depthFormat;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;
        
        result = vkCreateImageView(device->getLogicalDevice(), &viewInfo, nullptr, &depthImageView);
        if (result != VK_SUCCESS) {
            D6_THROW(VideoException, "Failed to create depth image view: " + std::to_string(result));
        }
    }

    void VulkanRenderPass::cleanup() {
        cleanupFramebuffers();
        cleanupDepthResources();
        
        if (renderPass != VK_NULL_HANDLE && device != nullptr) {
            vkDestroyRenderPass(device->getLogicalDevice(), renderPass, nullptr);
            renderPass = VK_NULL_HANDLE;
        }
    }

    void VulkanRenderPass::cleanupFramebuffers() {
        if (device != nullptr) {
            for (auto framebuffer : framebuffers) {
                vkDestroyFramebuffer(device->getLogicalDevice(), framebuffer, nullptr);
            }
        }
        framebuffers.clear();
        colorImageViews.clear();
    }

    void VulkanRenderPass::cleanupDepthResources() {
        if (device != nullptr) {
            if (depthImageView != VK_NULL_HANDLE) {
                vkDestroyImageView(device->getLogicalDevice(), depthImageView, nullptr);
                depthImageView = VK_NULL_HANDLE;
            }
            if (depthImage != VK_NULL_HANDLE) {
                vkDestroyImage(device->getLogicalDevice(), depthImage, nullptr);
                depthImage = VK_NULL_HANDLE;
            }
            if (depthImageMemory != VK_NULL_HANDLE) {
                vkFreeMemory(device->getLogicalDevice(), depthImageMemory, nullptr);
                depthImageMemory = VK_NULL_HANDLE;
            }
        }
    }

    // Command Buffer Manager implementation
    VulkanCommandBufferManager::VulkanCommandBufferManager(const VulkanDevice& device, uint32_t maxFramesInFlight)
        : device(&device), commandPool(VK_NULL_HANDLE), currentFrame(0), maxFramesInFlight(maxFramesInFlight) {
        createCommandPool();
        allocateCommandBuffers();
    }

    VulkanCommandBufferManager::~VulkanCommandBufferManager() {
        cleanup();
    }

    void VulkanCommandBufferManager::createCommandPool() {
        auto queueFamilyIndices = device->getQueueFamilyIndices();
        
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
        
        VkResult result = vkCreateCommandPool(device->getLogicalDevice(), &poolInfo, nullptr, &commandPool);
        if (result != VK_SUCCESS) {
            D6_THROW(VideoException, "Failed to create command pool: " + std::to_string(result));
        }
    }

    void VulkanCommandBufferManager::allocateCommandBuffers() {
        commandBuffers.resize(maxFramesInFlight);
        
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());
        
        VkResult result = vkAllocateCommandBuffers(device->getLogicalDevice(), &allocInfo, commandBuffers.data());
        if (result != VK_SUCCESS) {
            D6_THROW(VideoException, "Failed to allocate command buffers: " + std::to_string(result));
        }
    }

    VkCommandBuffer VulkanCommandBufferManager::getCurrentCommandBuffer() const {
        return commandBuffers[currentFrame];
    }

    void VulkanCommandBufferManager::beginFrame() {
        VkCommandBuffer commandBuffer = getCurrentCommandBuffer();
        
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0;
        beginInfo.pInheritanceInfo = nullptr;
        
        VkResult result = vkBeginCommandBuffer(commandBuffer, &beginInfo);
        if (result != VK_SUCCESS) {
            D6_THROW(VideoException, "Failed to begin recording command buffer: " + std::to_string(result));
        }
    }

    void VulkanCommandBufferManager::endFrame() {
        VkCommandBuffer commandBuffer = getCurrentCommandBuffer();
        
        VkResult result = vkEndCommandBuffer(commandBuffer);
        if (result != VK_SUCCESS) {
            D6_THROW(VideoException, "Failed to record command buffer: " + std::to_string(result));
        }
    }

    void VulkanCommandBufferManager::nextFrame() {
        currentFrame = (currentFrame + 1) % maxFramesInFlight;
    }

    void VulkanCommandBufferManager::reset() {
        VkCommandBuffer commandBuffer = getCurrentCommandBuffer();
        vkResetCommandBuffer(commandBuffer, 0);
    }

    void VulkanCommandBufferManager::submit(VkQueue queue, const std::vector<VkSemaphore>& waitSemaphores,
                                            const std::vector<VkPipelineStageFlags>& waitStages,
                                            const std::vector<VkSemaphore>& signalSemaphores,
                                            VkFence fence) const {
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        
        submitInfo.waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size());
        submitInfo.pWaitSemaphores = waitSemaphores.data();
        submitInfo.pWaitDstStageMask = waitStages.data();
        
        VkCommandBuffer commandBuffer = getCurrentCommandBuffer();
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;
        
        submitInfo.signalSemaphoreCount = static_cast<uint32_t>(signalSemaphores.size());
        submitInfo.pSignalSemaphores = signalSemaphores.data();
        
        VkResult result = vkQueueSubmit(queue, 1, &submitInfo, fence);
        if (result != VK_SUCCESS) {
            D6_THROW(VideoException, "Failed to submit draw command buffer: " + std::to_string(result));
        }
    }

    void VulkanCommandBufferManager::cleanup() {
        if (commandPool != VK_NULL_HANDLE && device != nullptr) {
            std::cerr << "VulkanCommandBufferManager: Destroying command pool: " << commandPool << std::endl;
            vkDestroyCommandPool(device->getLogicalDevice(), commandPool, nullptr);
            commandPool = VK_NULL_HANDLE;
        }
        commandBuffers.clear();
    }

    // Render Pass Manager implementation
    VulkanRenderPassManager::VulkanRenderPassManager(const VulkanDevice& device, uint32_t maxFramesInFlight)
        : device(&device) {
        commandBufferManager = std::make_unique<VulkanCommandBufferManager>(device, maxFramesInFlight);
    }

    void VulkanRenderPassManager::addRenderPass(std::unique_ptr<VulkanRenderPass> renderPass) {
        renderPasses.push_back(std::move(renderPass));
    }

    VulkanRenderPass* VulkanRenderPassManager::getRenderPass(size_t index) const {
        if (index >= renderPasses.size()) {
            return nullptr;
        }
        return renderPasses[index].get();
    }

    void VulkanRenderPassManager::beginFrame() {
        commandBufferManager->reset();
        commandBufferManager->beginFrame();
    }

    void VulkanRenderPassManager::endFrame() {
        commandBufferManager->endFrame();
        commandBufferManager->nextFrame();
    }

    void VulkanRenderPassManager::submit(VkQueue queue, const std::vector<VkSemaphore>& waitSemaphores,
                                         const std::vector<VkSemaphore>& signalSemaphores,
                                         VkFence fence) {
        std::vector<VkPipelineStageFlags> waitStages(waitSemaphores.size(), VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
        commandBufferManager->submit(queue, waitSemaphores, waitStages, signalSemaphores, fence);
    }

    void VulkanRenderPassManager::recreateFramebuffers(const VulkanSwapchain& swapchain) {
        for (auto& renderPass : renderPasses) {
            renderPass->recreateFramebuffers(swapchain);
        }
    }

    // Render Pass Presets implementation
    namespace RenderPassPresets {
        
        RenderPassConfig createMainSceneConfig(VkFormat swapchainFormat, VkFormat depthFormat) {
            RenderPassConfig config;
            config.colorFormat = swapchainFormat;
            config.depthFormat = depthFormat;
            config.hasDepthBuffer = true;
            config.clearColor = true;
            config.clearDepth = true;
            config.clearColorValue = {{0.0f, 0.0f, 0.2f, 1.0f}}; // Dark blue background
            return config;
        }
        
        RenderPassConfig createBloomConfig(VkFormat colorFormat) {
            RenderPassConfig config;
            config.colorFormat = colorFormat;
            config.hasDepthBuffer = false;
            config.hasBloomTarget = true;
            config.clearColor = true;
            config.clearColorValue = {{0.0f, 0.0f, 0.0f, 0.0f}}; // Transparent
            return config;
        }
        
        RenderPassConfig createPostProcessConfig(VkFormat colorFormat) {
            RenderPassConfig config;
            config.colorFormat = colorFormat;
            config.hasDepthBuffer = false;
            config.clearColor = true;
            config.clearColorValue = {{0.0f, 0.0f, 0.0f, 1.0f}}; // Black
            return config;
        }
        
        RenderPassConfig createShadowMapConfig(VkFormat depthFormat) {
            RenderPassConfig config;
            config.depthFormat = depthFormat;
            config.hasDepthBuffer = true;
            config.clearColor = false;
            config.clearDepth = true;
            return config;
        }
    }

}
