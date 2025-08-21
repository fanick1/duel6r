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

#include "VulkanSurface.h"
#include "../../Format.h"
#include <algorithm>

namespace Duel6 {
    VulkanSurface::VulkanSurface(Console& console)
        : surface(VK_NULL_HANDLE)
        , window(nullptr)
        , console(&console) {
    }

    VulkanSurface::~VulkanSurface() {
        // Surface cleanup should be handled via cleanup() method
    }

    void VulkanSurface::initialize(const VulkanContext& context, SDL_Window* window) {
        this->window = window;
        createSurface(context, window);
        console->printLine("...Vulkan surface created");
    }

    void VulkanSurface::cleanup(const VulkanContext& context) {
        if (surface != VK_NULL_HANDLE) {
            vkDestroySurfaceKHR(context.getInstance(), surface, nullptr);
            surface = VK_NULL_HANDLE;
        }
        window = nullptr;
    }

    void VulkanSurface::createSurface(const VulkanContext& context, SDL_Window* window) {
        if (!SDL_Vulkan_CreateSurface(window, context.getInstance(), &surface)) {
            D6_THROW(VulkanException, Format("Failed to create Vulkan surface: {0}") << SDL_GetError());
        }
    }

    SwapChainSupportDetails VulkanSurface::querySwapChainSupport(VkPhysicalDevice device) const {
        SwapChainSupportDetails details;

        // Surface capabilities
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

        // Surface formats
        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

        if (formatCount != 0) {
            details.formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
        }

        // Present modes
        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

        if (presentModeCount != 0) {
            details.presentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
        }

        return details;
    }

    VkSurfaceFormatKHR VulkanSurface::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) const {
        // Prefer SRGB color space for better color representation
        for (const auto& availableFormat : availableFormats) {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && 
                availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return availableFormat;
            }
        }

        // Fallback to UNORM if SRGB is not available
        for (const auto& availableFormat : availableFormats) {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && 
                availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return availableFormat;
            }
        }

        // If nothing specific is available, return the first one
        return availableFormats[0];
    }

    VkPresentModeKHR VulkanSurface::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) const {
        // Prefer triple buffering (MAILBOX) for best performance without tearing
        for (const auto& availablePresentMode : availablePresentModes) {
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                console->printLine("...Using MAILBOX present mode (triple buffering)");
                return availablePresentMode;
            }
        }

        // IMMEDIATE mode for lowest latency (may cause tearing)
        for (const auto& availablePresentMode : availablePresentModes) {
            if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
                console->printLine("...Using IMMEDIATE present mode");
                return availablePresentMode;
            }
        }

        // FIFO is always available and provides vsync
        console->printLine("...Using FIFO present mode (vsync)");
        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D VulkanSurface::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) const {
        if (capabilities.currentExtent.width != UINT32_MAX) {
            return capabilities.currentExtent;
        } else {
            // Get window size from SDL
            int width, height;
            SDL_GetWindowSize(window, &width, &height);

            VkExtent2D actualExtent = {
                static_cast<uint32_t>(width),
                static_cast<uint32_t>(height)
            };

            // Clamp to supported range
            actualExtent.width = std::clamp(actualExtent.width, 
                                          capabilities.minImageExtent.width, 
                                          capabilities.maxImageExtent.width);
            actualExtent.height = std::clamp(actualExtent.height, 
                                           capabilities.minImageExtent.height, 
                                           capabilities.maxImageExtent.height);

            return actualExtent;
        }
    }
}