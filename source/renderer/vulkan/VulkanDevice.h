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

#ifndef DUEL6_RENDERER_VULKANDEVICE_H
#define DUEL6_RENDERER_VULKANDEVICE_H

#include <vulkan/vulkan.h>
#include <vector>
#include <optional>
#include "VulkanContext.h"
#include "../../console/Console.h"

namespace Duel6 {
    struct QueueFamilyIndices {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;

        bool isComplete() const {
            return graphicsFamily.has_value() && presentFamily.has_value();
        }
    };

    class VulkanDevice {
    private:
        VkPhysicalDevice physicalDevice;
        VkDevice logicalDevice;
        VkQueue graphicsQueue;
        VkQueue presentQueue;
        QueueFamilyIndices queueFamilyIndices;
        VkCommandPool graphicsCommandPool;
        Console* console;

    public:
        explicit VulkanDevice(Console& console);
        ~VulkanDevice();

        // Non-copyable
        VulkanDevice(const VulkanDevice&) = delete;
        VulkanDevice& operator=(const VulkanDevice&) = delete;

        void initialize(const VulkanContext& context, VkSurfaceKHR surface);
        void cleanup();
        void waitIdle() const;

        // Getters
        VkPhysicalDevice getPhysicalDevice() const { return physicalDevice; }
        VkDevice getLogicalDevice() const { return logicalDevice; }
        VkQueue getGraphicsQueue() const { return graphicsQueue; }
        VkQueue getPresentQueue() const { return presentQueue; }
        const QueueFamilyIndices& getQueueFamilyIndices() const { return queueFamilyIndices; }

        // Device properties and capabilities
        VkPhysicalDeviceProperties getDeviceProperties() const;
        VkPhysicalDeviceMemoryProperties getMemoryProperties() const;
        uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;

        // Command buffer management
        VkCommandBuffer beginSingleTimeCommands() const;
        void endSingleTimeCommands(VkCommandBuffer commandBuffer) const;

    private:
        QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface) const;
        void createLogicalDevice();
        void createCommandPools();
        std::vector<const char*> getRequiredDeviceExtensions() const;
    };
}

#endif