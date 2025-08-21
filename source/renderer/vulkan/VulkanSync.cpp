/*
* Copyright (c) 2006, Ondrej Danek (www.ondrej-danek.net)
* All rights reserved.
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright
*       notice, this list of conditions and the following disclaimer in the
*       documentation and/or other materials provided with the distribution.
*     * Neither the name of Ondrej Danek nor the
*       names of its contributors may be used to endorse or promote products
*       derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
* GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
* LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
* OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "VulkanSync.h"
#include "VulkanContext.h"
#include "../../Format.h"

namespace Duel6 {
    VulkanSyncManager::VulkanSyncManager(Console& console)
        : maxFramesInFlight(0), currentFrame(0), console(&console), 
          srcStageFlags(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT), dstStageFlags(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT) {
    }

    VulkanSyncManager::~VulkanSyncManager() {
        if (!frameSyncObjects.empty()) {
            console->printLine("Warning: VulkanSyncManager destroyed without cleanup");
        }
    }

    void VulkanSyncManager::initialize(const VulkanDevice& device, uint32_t framesInFlight) {
        maxFramesInFlight = framesInFlight;
        currentFrame = 0;
        
        createFrameSyncObjects(device);
        
        console->printLine(Format("...Vulkan sync manager initialized with {0} frames in flight") << maxFramesInFlight);
    }

    void VulkanSyncManager::cleanup(const VulkanDevice& device) {
        waitForAllFrames(device);
        destroyFrameSyncObjects(device);
        
        imageStates.clear();
        bufferStates.clear();
        pendingImageBarriers.clear();
        pendingBufferBarriers.clear();
        
        maxFramesInFlight = 0;
        currentFrame = 0;
        
        console->printLine("...Vulkan sync manager cleaned up");
    }

    void VulkanSyncManager::waitForFrame(const VulkanDevice& device, uint32_t frameIndex) {
        if (frameIndex >= maxFramesInFlight) {
            return;
        }

        const FrameSync& frameSync = frameSyncObjects[frameIndex];
        if (frameSync.inFlightFence != VK_NULL_HANDLE) {
            vkWaitForFences(device.getLogicalDevice(), 1, &frameSync.inFlightFence, VK_TRUE, UINT64_MAX);
        }
    }

    void VulkanSyncManager::waitForAllFrames(const VulkanDevice& device) {
        for (uint32_t i = 0; i < maxFramesInFlight; ++i) {
            waitForFrame(device, i);
        }
    }

    void VulkanSyncManager::trackImageState(VkImage image, ImageLayout layout, VkAccessFlags access,
                                           VkPipelineStageFlags stage, uint32_t queueFamily) {
        ImageStateInfo& state = imageStates[image];
        state.image = image;
        state.currentLayout = layout;
        state.currentAccess = access;
        state.currentStage = stage;
        state.queueFamily = queueFamily;
    }

    void VulkanSyncManager::trackBufferState(VkBuffer buffer, VkAccessFlags access, VkPipelineStageFlags stage,
                                            uint32_t queueFamily) {
        BufferStateInfo& state = bufferStates[buffer];
        state.buffer = buffer;
        state.currentAccess = access;
        state.currentStage = stage;
        state.queueFamily = queueFamily;
    }

    void VulkanSyncManager::addImageBarrier(VkCommandBuffer commandBuffer, VkImage image,
                                           ImageLayout newLayout, VkAccessFlags newAccess, VkPipelineStageFlags newStage,
                                           uint32_t newQueueFamily) {
        auto it = imageStates.find(image);
        ImageStateInfo oldState;
        if (it != imageStates.end()) {
            oldState = it->second;
        }

        // Only add barrier if state actually changes
        if (oldState.currentLayout == newLayout && oldState.currentAccess == newAccess && 
            oldState.currentStage == newStage && oldState.queueFamily == newQueueFamily) {
            return;
        }

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = toVulkanImageLayout(oldState.currentLayout);
        barrier.newLayout = toVulkanImageLayout(newLayout);
        barrier.srcQueueFamilyIndex = oldState.queueFamily;
        barrier.dstQueueFamilyIndex = newQueueFamily;
        barrier.image = image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.srcAccessMask = oldState.currentAccess;
        barrier.dstAccessMask = newAccess;

        pendingImageBarriers.push_back(barrier);
        srcStageFlags |= oldState.currentStage;
        dstStageFlags |= newStage;

        // Update tracked state
        trackImageState(image, newLayout, newAccess, newStage, newQueueFamily);
    }

    void VulkanSyncManager::addBufferBarrier(VkCommandBuffer commandBuffer, VkBuffer buffer,
                                            VkAccessFlags newAccess, VkPipelineStageFlags newStage,
                                            uint32_t newQueueFamily) {
        auto it = bufferStates.find(buffer);
        BufferStateInfo oldState;
        if (it != bufferStates.end()) {
            oldState = it->second;
        }

        // Only add barrier if state actually changes
        if (oldState.currentAccess == newAccess && oldState.currentStage == newStage && 
            oldState.queueFamily == newQueueFamily) {
            return;
        }

        VkBufferMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        barrier.srcAccessMask = oldState.currentAccess;
        barrier.dstAccessMask = newAccess;
        barrier.srcQueueFamilyIndex = oldState.queueFamily;
        barrier.dstQueueFamilyIndex = newQueueFamily;
        barrier.buffer = buffer;
        barrier.offset = 0;
        barrier.size = VK_WHOLE_SIZE;

        pendingBufferBarriers.push_back(barrier);
        srcStageFlags |= oldState.currentStage;
        dstStageFlags |= newStage;

        // Update tracked state
        trackBufferState(buffer, newAccess, newStage, newQueueFamily);
    }

    void VulkanSyncManager::flushBarriers(VkCommandBuffer commandBuffer) {
        if (pendingImageBarriers.empty() && pendingBufferBarriers.empty()) {
            return;
        }

        vkCmdPipelineBarrier(commandBuffer, srcStageFlags, dstStageFlags, 0,
                            0, nullptr,
                            static_cast<uint32_t>(pendingBufferBarriers.size()), pendingBufferBarriers.data(),
                            static_cast<uint32_t>(pendingImageBarriers.size()), pendingImageBarriers.data());

        // Clear pending barriers
        pendingImageBarriers.clear();
        pendingBufferBarriers.clear();
        srcStageFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dstStageFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    }

    void VulkanSyncManager::transitionImageForPresent(VkCommandBuffer commandBuffer, VkImage image) {
        addImageBarrier(commandBuffer, image, ImageLayout::Present, 0, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
        flushBarriers(commandBuffer);
    }

    void VulkanSyncManager::transitionImageForColorAttachment(VkCommandBuffer commandBuffer, VkImage image) {
        addImageBarrier(commandBuffer, image, ImageLayout::ColorAttachment, 
                       VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                       VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
        flushBarriers(commandBuffer);
    }

    void VulkanSyncManager::transitionImageForShaderRead(VkCommandBuffer commandBuffer, VkImage image) {
        addImageBarrier(commandBuffer, image, ImageLayout::ShaderReadOnly, 
                       VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
        flushBarriers(commandBuffer);
    }

    void VulkanSyncManager::transitionImageForTransfer(VkCommandBuffer commandBuffer, VkImage image, bool source) {
        if (source) {
            addImageBarrier(commandBuffer, image, ImageLayout::TransferSrc, 
                           VK_ACCESS_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
        } else {
            addImageBarrier(commandBuffer, image, ImageLayout::TransferDst, 
                           VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
        }
        flushBarriers(commandBuffer);
    }

    void VulkanSyncManager::resetImageState(VkImage image) {
        imageStates.erase(image);
    }

    void VulkanSyncManager::resetBufferState(VkBuffer buffer) {
        bufferStates.erase(buffer);
    }

    void VulkanSyncManager::clearAllStates() {
        imageStates.clear();
        bufferStates.clear();
    }

    VkImageLayout VulkanSyncManager::toVulkanImageLayout(ImageLayout layout) const {
        switch (layout) {
            case ImageLayout::Undefined: return VK_IMAGE_LAYOUT_UNDEFINED;
            case ImageLayout::General: return VK_IMAGE_LAYOUT_GENERAL;
            case ImageLayout::ColorAttachment: return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            case ImageLayout::DepthStencilAttachment: return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            case ImageLayout::ShaderReadOnly: return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            case ImageLayout::TransferSrc: return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            case ImageLayout::TransferDst: return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            case ImageLayout::Present: return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            default: return VK_IMAGE_LAYOUT_UNDEFINED;
        }
    }

    ImageLayout VulkanSyncManager::fromVulkanImageLayout(VkImageLayout layout) const {
        switch (layout) {
            case VK_IMAGE_LAYOUT_UNDEFINED: return ImageLayout::Undefined;
            case VK_IMAGE_LAYOUT_GENERAL: return ImageLayout::General;
            case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL: return ImageLayout::ColorAttachment;
            case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL: return ImageLayout::DepthStencilAttachment;
            case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL: return ImageLayout::ShaderReadOnly;
            case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL: return ImageLayout::TransferSrc;
            case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL: return ImageLayout::TransferDst;
            case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR: return ImageLayout::Present;
            default: return ImageLayout::Undefined;
        }
    }

    void VulkanSyncManager::createFrameSyncObjects(const VulkanDevice& device) {
        frameSyncObjects.resize(maxFramesInFlight);

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // Start signaled

        for (uint32_t i = 0; i < maxFramesInFlight; ++i) {
            FrameSync& frameSync = frameSyncObjects[i];

            VkResult result = vkCreateSemaphore(device.getLogicalDevice(), &semaphoreInfo, nullptr, &frameSync.imageAvailableSemaphore);
            if (result != VK_SUCCESS) {
                D6_THROW(VulkanException, Format("Failed to create image available semaphore {0}, error: {1}") << i << result);
            }

            result = vkCreateSemaphore(device.getLogicalDevice(), &semaphoreInfo, nullptr, &frameSync.renderFinishedSemaphore);
            if (result != VK_SUCCESS) {
                D6_THROW(VulkanException, Format("Failed to create render finished semaphore {0}, error: {1}") << i << result);
            }

            result = vkCreateFence(device.getLogicalDevice(), &fenceInfo, nullptr, &frameSync.inFlightFence);
            if (result != VK_SUCCESS) {
                D6_THROW(VulkanException, Format("Failed to create in-flight fence {0}, error: {1}") << i << result);
            }
        }
    }

    void VulkanSyncManager::destroyFrameSyncObjects(const VulkanDevice& device) {
        for (FrameSync& frameSync : frameSyncObjects) {
            if (frameSync.imageAvailableSemaphore != VK_NULL_HANDLE) {
                vkDestroySemaphore(device.getLogicalDevice(), frameSync.imageAvailableSemaphore, nullptr);
                frameSync.imageAvailableSemaphore = VK_NULL_HANDLE;
            }

            if (frameSync.renderFinishedSemaphore != VK_NULL_HANDLE) {
                vkDestroySemaphore(device.getLogicalDevice(), frameSync.renderFinishedSemaphore, nullptr);
                frameSync.renderFinishedSemaphore = VK_NULL_HANDLE;
            }

            if (frameSync.inFlightFence != VK_NULL_HANDLE) {
                vkDestroyFence(device.getLogicalDevice(), frameSync.inFlightFence, nullptr);
                frameSync.inFlightFence = VK_NULL_HANDLE;
            }
        }
        frameSyncObjects.clear();
    }

    // ScopedImageTransition implementation
    ScopedImageTransition::ScopedImageTransition(VulkanSyncManager* sync, VkCommandBuffer cmd, VkImage img,
                                                 ImageLayout newLayout, VkAccessFlags newAccess, VkPipelineStageFlags newStage)
        : syncManager(sync), commandBuffer(cmd), image(img) {
        
        // Store original state
        auto it = syncManager->imageStates.find(image);
        if (it != syncManager->imageStates.end()) {
            originalLayout = it->second.currentLayout;
            originalAccess = it->second.currentAccess;
            originalStage = it->second.currentStage;
        } else {
            originalLayout = ImageLayout::Undefined;
            originalAccess = 0;
            originalStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        }

        // Transition to new state
        syncManager->addImageBarrier(commandBuffer, image, newLayout, newAccess, newStage);
        syncManager->flushBarriers(commandBuffer);
    }

    ScopedImageTransition::~ScopedImageTransition() {
        // Transition back to original state
        syncManager->addImageBarrier(commandBuffer, image, originalLayout, originalAccess, originalStage);
        syncManager->flushBarriers(commandBuffer);
    }
}