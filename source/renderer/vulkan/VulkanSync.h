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

#ifndef DUEL6_RENDERER_VULKANSYNC_H
#define DUEL6_RENDERER_VULKANSYNC_H

#include <vulkan/vulkan.h>
#include <vector>
#include <memory>
#include <unordered_map>
#include "VulkanDevice.h"
#include "../../console/Console.h"

namespace Duel6 {
    // Frame synchronization objects
    struct FrameSync {
        VkSemaphore imageAvailableSemaphore;
        VkSemaphore renderFinishedSemaphore;
        VkFence inFlightFence;

        FrameSync() : imageAvailableSemaphore(VK_NULL_HANDLE), renderFinishedSemaphore(VK_NULL_HANDLE), 
                     inFlightFence(VK_NULL_HANDLE) {}
    };

    // Resource state tracking for barriers
    enum class ImageLayout {
        Undefined,
        General,
        ColorAttachment,
        DepthStencilAttachment,
        ShaderReadOnly,
        TransferSrc,
        TransferDst,
        Present
    };

    struct ImageStateInfo {
        VkImage image;
        ImageLayout currentLayout;
        VkAccessFlags currentAccess;
        VkPipelineStageFlags currentStage;
        uint32_t queueFamily;

        ImageStateInfo() : image(VK_NULL_HANDLE), currentLayout(ImageLayout::Undefined), 
                          currentAccess(0), currentStage(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT), 
                          queueFamily(VK_QUEUE_FAMILY_IGNORED) {}
    };

    struct BufferStateInfo {
        VkBuffer buffer;
        VkAccessFlags currentAccess;
        VkPipelineStageFlags currentStage;
        uint32_t queueFamily;

        BufferStateInfo() : buffer(VK_NULL_HANDLE), currentAccess(0), 
                           currentStage(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT), 
                           queueFamily(VK_QUEUE_FAMILY_IGNORED) {}
    };

    class VulkanSyncManager {
        friend class ScopedImageTransition;
        
    private:
        std::vector<FrameSync> frameSyncObjects;
        std::unordered_map<VkImage, ImageStateInfo> imageStates;
        std::unordered_map<VkBuffer, BufferStateInfo> bufferStates;
        uint32_t maxFramesInFlight;
        uint32_t currentFrame;
        Console* console;

    public:
        explicit VulkanSyncManager(Console& console);
        ~VulkanSyncManager();

        // Non-copyable
        VulkanSyncManager(const VulkanSyncManager&) = delete;
        VulkanSyncManager& operator=(const VulkanSyncManager&) = delete;

        void initialize(const VulkanDevice& device, uint32_t framesInFlight = 2);
        void cleanup(const VulkanDevice& device);

        // Frame synchronization
        FrameSync& getCurrentFrameSync() { return frameSyncObjects[currentFrame]; }
        const FrameSync& getCurrentFrameSync() const { return frameSyncObjects[currentFrame]; }
        void nextFrame() { currentFrame = (currentFrame + 1) % maxFramesInFlight; }
        uint32_t getCurrentFrameIndex() const { return currentFrame; }
        uint32_t getMaxFramesInFlight() const { return maxFramesInFlight; }

        // Wait for frame completion
        void waitForFrame(const VulkanDevice& device, uint32_t frameIndex);
        void waitForAllFrames(const VulkanDevice& device);

        // Resource state tracking
        void trackImageState(VkImage image, ImageLayout layout, VkAccessFlags access, 
                            VkPipelineStageFlags stage, uint32_t queueFamily = VK_QUEUE_FAMILY_IGNORED);
        void trackBufferState(VkBuffer buffer, VkAccessFlags access, VkPipelineStageFlags stage,
                             uint32_t queueFamily = VK_QUEUE_FAMILY_IGNORED);

        // Barrier generation
        void addImageBarrier(VkCommandBuffer commandBuffer, VkImage image, 
                            ImageLayout newLayout, VkAccessFlags newAccess, VkPipelineStageFlags newStage,
                            uint32_t newQueueFamily = VK_QUEUE_FAMILY_IGNORED);
        void addBufferBarrier(VkCommandBuffer commandBuffer, VkBuffer buffer,
                             VkAccessFlags newAccess, VkPipelineStageFlags newStage,
                             uint32_t newQueueFamily = VK_QUEUE_FAMILY_IGNORED);

        // Batch barrier submission
        void flushBarriers(VkCommandBuffer commandBuffer);

        // Common transitions
        void transitionImageForPresent(VkCommandBuffer commandBuffer, VkImage image);
        void transitionImageForColorAttachment(VkCommandBuffer commandBuffer, VkImage image);
        void transitionImageForShaderRead(VkCommandBuffer commandBuffer, VkImage image);
        void transitionImageForTransfer(VkCommandBuffer commandBuffer, VkImage image, bool source = false);

        // Reset state tracking (call when resources are recreated)
        void resetImageState(VkImage image);
        void resetBufferState(VkBuffer buffer);
        void clearAllStates();

    private:
        VkImageLayout toVulkanImageLayout(ImageLayout layout) const;
        ImageLayout fromVulkanImageLayout(VkImageLayout layout) const;
        
        void createFrameSyncObjects(const VulkanDevice& device);
        void destroyFrameSyncObjects(const VulkanDevice& device);

        // Pending barriers for batch submission
        std::vector<VkImageMemoryBarrier> pendingImageBarriers;
        std::vector<VkBufferMemoryBarrier> pendingBufferBarriers;
        VkPipelineStageFlags srcStageFlags;
        VkPipelineStageFlags dstStageFlags;
    };

    // Utility class for automatic resource state management
    class ScopedImageTransition {
    private:
        VulkanSyncManager* syncManager;
        VkCommandBuffer commandBuffer;
        VkImage image;
        ImageLayout originalLayout;
        VkAccessFlags originalAccess;
        VkPipelineStageFlags originalStage;

    public:
        ScopedImageTransition(VulkanSyncManager* sync, VkCommandBuffer cmd, VkImage img, 
                             ImageLayout newLayout, VkAccessFlags newAccess, VkPipelineStageFlags newStage);
        ~ScopedImageTransition();

        // Non-copyable
        ScopedImageTransition(const ScopedImageTransition&) = delete;
        ScopedImageTransition& operator=(const ScopedImageTransition&) = delete;
    };
}

#endif