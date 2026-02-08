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

#include "VulkanSwapchain.h"
#include "../../Format.h"
#include <algorithm>

namespace Duel6 {
    VulkanSwapchain::VulkanSwapchain(Console& console)
        : swapchain(VK_NULL_HANDLE)
        , swapchainImageFormat(VK_FORMAT_UNDEFINED)
        , swapchainExtent({0, 0})
        , currentFrame(0)
        , imageIndex(0)
        , console(&console) {
    }

    VulkanSwapchain::~VulkanSwapchain() {
        // Cleanup should be handled via cleanup() method
    }

    void VulkanSwapchain::initialize(const VulkanDevice& device, const VulkanSurface& surface) {
        createSwapchain(device, surface);
        createImageViews(device);
        createSyncObjects(device);

        console->printLine(Format("...Vulkan swapchain initialized: {0}x{1}, {2} images") 
            << swapchainExtent.width << swapchainExtent.height << swapchainImages.size());
    }

    void VulkanSwapchain::cleanup(const VulkanDevice& device) {
        device.waitIdle();
        cleanupSwapchain(device);

        // Cleanup image available semaphores (per-frame)
        for (size_t i = 0; i < imageAvailableSemaphores.size(); i++) {
            if (imageAvailableSemaphores[i] != VK_NULL_HANDLE) {
                vkDestroySemaphore(device.getLogicalDevice(), imageAvailableSemaphores[i], nullptr);
            }
        }
        
        // Cleanup render finished semaphores (per-image)
        for (size_t i = 0; i < renderFinishedSemaphores.size(); i++) {
            if (renderFinishedSemaphores[i] != VK_NULL_HANDLE) {
                vkDestroySemaphore(device.getLogicalDevice(), renderFinishedSemaphores[i], nullptr);
            }
        }
        
        // Cleanup fences (per-frame)
        for (size_t i = 0; i < inFlightFences.size(); i++) {
            if (inFlightFences[i] != VK_NULL_HANDLE) {
                vkDestroyFence(device.getLogicalDevice(), inFlightFences[i], nullptr);
            }
        }

        imageAvailableSemaphores.clear();
        renderFinishedSemaphores.clear();
        inFlightFences.clear();
        imagesInFlight.clear();
    }

    void VulkanSwapchain::recreate(const VulkanDevice& device, const VulkanSurface& surface) {
        device.waitIdle();
        
        cleanupSwapchain(device);
        
        createSwapchain(device, surface);
        createImageViews(device);
        
        imagesInFlight.resize(swapchainImages.size(), VK_NULL_HANDLE);

        console->printLine("...Vulkan swapchain recreated");
    }

    void VulkanSwapchain::createSwapchain(const VulkanDevice& device, const VulkanSurface& surface) {
        SwapChainSupportDetails swapChainSupport = surface.querySwapChainSupport(device.getPhysicalDevice());

        VkSurfaceFormatKHR surfaceFormat = surface.chooseSwapSurfaceFormat(swapChainSupport.formats);
        VkPresentModeKHR presentMode = surface.chooseSwapPresentMode(swapChainSupport.presentModes);
        VkExtent2D extent = surface.chooseSwapExtent(swapChainSupport.capabilities);

        // Request one more than minimum to implement triple buffering
        uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
        if (swapChainSupport.capabilities.maxImageCount > 0 && 
            imageCount > swapChainSupport.capabilities.maxImageCount) {
            imageCount = swapChainSupport.capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = surface.getSurface();
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        QueueFamilyIndices indices = device.getQueueFamilyIndices();
        uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

        if (indices.graphicsFamily != indices.presentFamily) {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        } else {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount = 0; // Optional
            createInfo.pQueueFamilyIndices = nullptr; // Optional
        }

        createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;
        createInfo.oldSwapchain = VK_NULL_HANDLE;

        VkResult result = vkCreateSwapchainKHR(device.getLogicalDevice(), &createInfo, nullptr, &swapchain);
        if (result != VK_SUCCESS) {
            D6_THROW(VulkanException, Format("Failed to create swap chain, error: {0}") << result);
        }

        // Get swapchain images
        vkGetSwapchainImagesKHR(device.getLogicalDevice(), swapchain, &imageCount, nullptr);
        swapchainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(device.getLogicalDevice(), swapchain, &imageCount, swapchainImages.data());

        swapchainImageFormat = surfaceFormat.format;
        swapchainExtent = extent;
    }

    void VulkanSwapchain::createImageViews(const VulkanDevice& device) {
        swapchainImageViews.resize(swapchainImages.size());

        for (size_t i = 0; i < swapchainImages.size(); i++) {
            VkImageViewCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.image = swapchainImages[i];
            createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            createInfo.format = swapchainImageFormat;
            createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseMipLevel = 0;
            createInfo.subresourceRange.levelCount = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount = 1;

            VkResult result = vkCreateImageView(device.getLogicalDevice(), &createInfo, nullptr, &swapchainImageViews[i]);
            if (result != VK_SUCCESS) {
                D6_THROW(VulkanException, Format("Failed to create image view {0}, error: {1}") << i << result);
            }
        }
    }

    void VulkanSwapchain::createSyncObjects(const VulkanDevice& device) {
        // Image available semaphores: per-frame in flight (we don't know imageIndex during acquisition)
        imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        // Render finished semaphores: per-image (prevents reuse conflicts)
        renderFinishedSemaphores.resize(swapchainImages.size());
        
        // Keep fences per frame in flight for CPU-GPU synchronization
        inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
        imagesInFlight.resize(swapchainImages.size(), VK_NULL_HANDLE);

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        // Create image available semaphores for each frame in flight
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            VkResult result = vkCreateSemaphore(device.getLogicalDevice(), &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]);
            if (result != VK_SUCCESS) {
                D6_THROW(VulkanException, Format("Failed to create image available semaphore for frame {0}") << i);
            }
        }
        
        // Create render finished semaphores for each swapchain image
        for (size_t i = 0; i < swapchainImages.size(); i++) {
            VkResult result = vkCreateSemaphore(device.getLogicalDevice(), &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]);
            if (result != VK_SUCCESS) {
                D6_THROW(VulkanException, Format("Failed to create render finished semaphore for image {0}") << i);
            }
        }
        
        // Create fences for each frame in flight
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            VkResult result = vkCreateFence(device.getLogicalDevice(), &fenceInfo, nullptr, &inFlightFences[i]);
            if (result != VK_SUCCESS) {
                D6_THROW(VulkanException, Format("Failed to create fence for frame {0}") << i);
            }
        }
    }

    bool VulkanSwapchain::acquireNextImage(const VulkanDevice& device) {
        vkWaitForFences(device.getLogicalDevice(), 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

        // Use a reasonable timeout (1 second) instead of infinite wait
        const uint64_t timeout = 1000000000; // 1 second in nanoseconds
        
        VkResult result = vkAcquireNextImageKHR(device.getLogicalDevice(), swapchain, timeout, 
                                               imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            return false; // Swapchain needs recreation
        } else if (result == VK_TIMEOUT) {
            // Timeout occurred - try again
            return false; // Return false to indicate we should try again
        } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            D6_THROW(VulkanException, Format("Failed to acquire swap chain image, error: {0}") << result);
        }

        if (imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
            vkWaitForFences(device.getLogicalDevice(), 1, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
        }
        imagesInFlight[imageIndex] = inFlightFences[currentFrame];

        vkResetFences(device.getLogicalDevice(), 1, &inFlightFences[currentFrame]);

        return true;
    }

    void VulkanSwapchain::present(const VulkanDevice& device) {
        VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[imageIndex]};

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;

        VkSwapchainKHR swapChains[] = {swapchain};
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;
        presentInfo.pImageIndices = &imageIndex;
        presentInfo.pResults = nullptr; // Optional

        vkQueuePresentKHR(device.getPresentQueue(), &presentInfo);

        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    void VulkanSwapchain::cleanupSwapchain(const VulkanDevice& device) {
        for (auto imageView : swapchainImageViews) {
            if (imageView != VK_NULL_HANDLE) {
                vkDestroyImageView(device.getLogicalDevice(), imageView, nullptr);
            }
        }
        swapchainImageViews.clear();

        if (swapchain != VK_NULL_HANDLE) {
            vkDestroySwapchainKHR(device.getLogicalDevice(), swapchain, nullptr);
            swapchain = VK_NULL_HANDLE;
        }

        swapchainImages.clear();
    }
}