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

#ifndef DUEL6_RENDERER_VULKAN_RENDERPASS_H
#define DUEL6_RENDERER_VULKAN_RENDERPASS_H

#include <vulkan/vulkan.h>
#include <vector>

namespace Duel6 {

    // Forward declarations
    class VulkanDevice;
    class VulkanSwapchain;

    /**
     * Vulkan render pass configuration
     */
    struct RenderPassConfig {
        VkFormat colorFormat = VK_FORMAT_B8G8R8A8_UNORM;
        VkFormat depthFormat = VK_FORMAT_D32_SFLOAT_S8_UINT;
        VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;
        bool hasDepthBuffer = true;
        bool clearColor = true;
        bool clearDepth = true;
        VkClearColorValue clearColorValue = {{0.0f, 0.0f, 0.0f, 1.0f}};
        float clearDepthValue = 1.0f;
        uint32_t stencilClearValue = 0;
    };

    /**
     * Vulkan render pass wrapper
     */
    class VulkanRenderPass {
    private:
        const VulkanDevice* device;
        VkRenderPass renderPass;
        RenderPassConfig config;
        
        // Framebuffer management
        std::vector<VkFramebuffer> framebuffers;
        std::vector<VkImageView> colorImageViews;
        VkImage depthImage;
        VkImageView depthImageView;
        VkDeviceMemory depthImageMemory;
        
        uint32_t width;
        uint32_t height;

    public:
        VulkanRenderPass(const VulkanDevice& device, const RenderPassConfig& config);
        ~VulkanRenderPass();

        // Non-copyable
        VulkanRenderPass(const VulkanRenderPass&) = delete;
        VulkanRenderPass& operator=(const VulkanRenderPass&) = delete;

        /**
         * Create framebuffers for swapchain images
         */
        void createFramebuffers(const VulkanSwapchain& swapchain);

        /**
         * Recreate framebuffers (e.g., on swapchain recreation)
         */
        void recreateFramebuffers(const VulkanSwapchain& swapchain);

        /**
         * Begin render pass
         */
        void begin(VkCommandBuffer commandBuffer, uint32_t framebufferIndex, 
                   VkSubpassContents contents = VK_SUBPASS_CONTENTS_INLINE) const;

        /**
         * End render pass
         */
        void end(VkCommandBuffer commandBuffer) const;

        // Getters
        VkRenderPass getHandle() const { return renderPass; }
        const RenderPassConfig& getConfig() const { return config; }

    private:
        void createRenderPass();
        void createDepthResources();
        void cleanup();
        void cleanupFramebuffers();
        void cleanupDepthResources();
    };


    /**
     * Command buffer manager for render pass recording
     */
    class VulkanCommandBufferManager {
    private:
        const VulkanDevice* device;
        VkCommandPool commandPool;
        std::vector<VkCommandBuffer> commandBuffers;
        uint32_t currentFrame;
        uint32_t maxFramesInFlight;

    public:
        VulkanCommandBufferManager(const VulkanDevice& device, uint32_t maxFramesInFlight = 2);
        ~VulkanCommandBufferManager();

        // Non-copyable
        VulkanCommandBufferManager(const VulkanCommandBufferManager&) = delete;
        VulkanCommandBufferManager& operator=(const VulkanCommandBufferManager&) = delete;

        /**
         * Get command buffer for current frame
         */
        VkCommandBuffer getCurrentCommandBuffer() const;

        /**
         * Begin recording commands for current frame
         */
        void beginFrame();

        /**
         * End recording commands for current frame
         */
        void endFrame();

        /**
         * Advance to next frame
         */
        void nextFrame();

        /**
         * Reset command buffer for current frame
         */

        /**
         * Submit command buffer to queue
         */
        void submit(VkQueue queue, const std::vector<VkSemaphore>& waitSemaphores = {},
                    const std::vector<VkPipelineStageFlags>& waitStages = {},
                    const std::vector<VkSemaphore>& signalSemaphores = {},
                    VkFence fence = VK_NULL_HANDLE) const;

        // Getters

    private:
        void createCommandPool();
        void allocateCommandBuffers();
        void cleanup();
    };


}

#endif
