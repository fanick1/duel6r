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

#ifndef DUEL6_RENDERER_VULKANSWAPCHAIN_H
#define DUEL6_RENDERER_VULKANSWAPCHAIN_H

#include <vulkan/vulkan.h>
#include <vector>
#include "VulkanDevice.h"
#include "VulkanSurface.h"
#include "../../console/Console.h"

namespace Duel6 {
    class VulkanSwapchain {
    private:
        VkSwapchainKHR swapchain;
        std::vector<VkImage> swapchainImages;
        std::vector<VkImageView> swapchainImageViews;
        VkFormat swapchainImageFormat;
        VkExtent2D swapchainExtent;
        
        // Synchronization objects
        std::vector<VkSemaphore> imageAvailableSemaphores;
        std::vector<VkSemaphore> renderFinishedSemaphores;
        std::vector<VkFence> inFlightFences;
        std::vector<VkFence> imagesInFlight;
        
        size_t currentFrame;
        uint32_t imageIndex;
        static const int MAX_FRAMES_IN_FLIGHT = 2;
        
        Console* console;

    public:
        explicit VulkanSwapchain(Console& console);
        ~VulkanSwapchain();

        // Non-copyable
        VulkanSwapchain(const VulkanSwapchain&) = delete;
        VulkanSwapchain& operator=(const VulkanSwapchain&) = delete;

        void initialize(const VulkanDevice& device, const VulkanSurface& surface);
        void cleanup(const VulkanDevice& device);
        void recreate(const VulkanDevice& device, const VulkanSurface& surface);

        // Frame operations
        bool acquireNextImage(const VulkanDevice& device);
        void present(const VulkanDevice& device);

        // Getters
        VkSwapchainKHR getSwapchain() const { return swapchain; }
        VkFormat getImageFormat() const { return swapchainImageFormat; }
        VkExtent2D getExtent() const { return swapchainExtent; }
        // Use currentFrame for acquisition semaphore (before we know imageIndex)
        VkSemaphore getImageAvailableSemaphore() const { return imageAvailableSemaphores[currentFrame]; }
        // Use imageIndex for render finished semaphore (after we know imageIndex)  
        VkSemaphore getRenderFinishedSemaphore() const { return renderFinishedSemaphores[imageIndex]; }
        VkFence getInFlightFence() const { return inFlightFences[currentFrame]; }
        uint32_t getCurrentImageIndex() const { return imageIndex; }
        VkImage getImage(uint32_t index) const { return swapchainImages[index]; }
        const std::vector<VkImageView>& getImageViews() const { return swapchainImageViews; }

    private:
        void createSwapchain(const VulkanDevice& device, const VulkanSurface& surface);
        void createImageViews(const VulkanDevice& device);
        void createSyncObjects(const VulkanDevice& device);
        void cleanupSwapchain(const VulkanDevice& device);
    };
}

#endif