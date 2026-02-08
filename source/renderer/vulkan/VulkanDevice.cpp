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

#include "VulkanDevice.h"
#include "../../Format.h"
#include <set>

namespace Duel6 {
    VulkanDevice::VulkanDevice(Console& console)
        : physicalDevice(VK_NULL_HANDLE)
        , logicalDevice(VK_NULL_HANDLE)
        , graphicsQueue(VK_NULL_HANDLE)
        , presentQueue(VK_NULL_HANDLE)
        , transferQueue(VK_NULL_HANDLE)
        , graphicsCommandPool(VK_NULL_HANDLE)
        , transferCommandPool(VK_NULL_HANDLE)
        , console(&console) {
    }

    VulkanDevice::~VulkanDevice() {
        cleanup();
    }

    void VulkanDevice::initialize(const VulkanContext& context, VkSurfaceKHR surface) {
        // Select physical device
        physicalDevice = context.selectPhysicalDevice(surface);
        queueFamilyIndices = findQueueFamilies(physicalDevice, surface);

        // Create logical device
        createLogicalDevice(context);

        // Create command pools
        createCommandPools();

        VkPhysicalDeviceProperties properties = getDeviceProperties();
        console->printLine(Format("...Vulkan device initialized: {0}") << properties.deviceName);
    }

    void VulkanDevice::cleanup() {
        if (logicalDevice != VK_NULL_HANDLE) {
            vkDeviceWaitIdle(logicalDevice);

            // Store the original graphics command pool handle before nullifying it
            VkCommandPool originalGraphicsCommandPool = graphicsCommandPool;
            
            if (graphicsCommandPool != VK_NULL_HANDLE) {
                vkDestroyCommandPool(logicalDevice, graphicsCommandPool, nullptr);
                graphicsCommandPool = VK_NULL_HANDLE;
            }

            if (transferCommandPool != VK_NULL_HANDLE && transferCommandPool != originalGraphicsCommandPool) {
                vkDestroyCommandPool(logicalDevice, transferCommandPool, nullptr);
            }
            transferCommandPool = VK_NULL_HANDLE;

            vkDestroyDevice(logicalDevice, nullptr);
            logicalDevice = VK_NULL_HANDLE;
        }

        physicalDevice = VK_NULL_HANDLE;
        graphicsQueue = VK_NULL_HANDLE;
        presentQueue = VK_NULL_HANDLE;
        transferQueue = VK_NULL_HANDLE;
    }

    void VulkanDevice::waitIdle() const {
        if (logicalDevice != VK_NULL_HANDLE) {
            vkDeviceWaitIdle(logicalDevice);
        }
    }

    QueueFamilyIndices VulkanDevice::findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface) const {
        QueueFamilyIndices indices;

        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        int i = 0;
        for (const auto& queueFamily : queueFamilies) {
            // Graphics queue
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                indices.graphicsFamily = i;
            }

            // Present queue
            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
            if (presentSupport) {
                indices.presentFamily = i;
            }

            // Transfer queue (prefer dedicated transfer queue)
            if ((queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT) && 
                !(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
                indices.transferFamily = i;
            }

            if (indices.isComplete()) {
                break;
            }
            i++;
        }

        // If no dedicated transfer queue, use graphics queue
        if (!indices.transferFamily.has_value() && indices.graphicsFamily.has_value()) {
            indices.transferFamily = indices.graphicsFamily;
        }

        return indices;
    }

    void VulkanDevice::createLogicalDevice(const VulkanContext& context) {
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set<uint32_t> uniqueQueueFamilies = {
            queueFamilyIndices.graphicsFamily.value(),
            queueFamilyIndices.presentFamily.value()
        };

        if (queueFamilyIndices.transferFamily.has_value()) {
            uniqueQueueFamilies.insert(queueFamilyIndices.transferFamily.value());
        }

        float queuePriority = 1.0f;
        for (uint32_t queueFamily : uniqueQueueFamilies) {
            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }

        // Query available device features
        VkPhysicalDeviceFeatures availableFeatures;
        vkGetPhysicalDeviceFeatures(physicalDevice, &availableFeatures);
        
        VkPhysicalDeviceFeatures deviceFeatures{};
        // Enable features only if available
        if (availableFeatures.samplerAnisotropy) {
            deviceFeatures.samplerAnisotropy = VK_TRUE;
        }
        if (availableFeatures.fillModeNonSolid) {
            deviceFeatures.fillModeNonSolid = VK_TRUE;
        }

        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pQueueCreateInfos = queueCreateInfos.data();
        createInfo.pEnabledFeatures = &deviceFeatures;

        auto extensions = getRequiredDeviceExtensions();
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();

        // Device-level validation layers are deprecated since Vulkan 1.0.13
        // Validation is now only set up at the instance level
        createInfo.enabledLayerCount = 0;
        createInfo.ppEnabledLayerNames = nullptr;

        VkResult result = vkCreateDevice(physicalDevice, &createInfo, nullptr, &logicalDevice);
        if (result != VK_SUCCESS) {
            D6_THROW(VulkanException, Format("Failed to create logical device, error: {0}") << result);
        }

        // Retrieve queue handles
        vkGetDeviceQueue(logicalDevice, queueFamilyIndices.graphicsFamily.value(), 0, &graphicsQueue);
        vkGetDeviceQueue(logicalDevice, queueFamilyIndices.presentFamily.value(), 0, &presentQueue);
        
        if (queueFamilyIndices.transferFamily.has_value()) {
            vkGetDeviceQueue(logicalDevice, queueFamilyIndices.transferFamily.value(), 0, &transferQueue);
        } else {
            transferQueue = graphicsQueue;
        }

        console->printLine("...Vulkan logical device created");
        console->printLine(Format("...Graphics queue family: {0}") << queueFamilyIndices.graphicsFamily.value());
        console->printLine(Format("...Present queue family: {0}") << queueFamilyIndices.presentFamily.value());
        if (queueFamilyIndices.transferFamily.has_value()) {
            console->printLine(Format("...Transfer queue family: {0}") << queueFamilyIndices.transferFamily.value());
        }
    }

    void VulkanDevice::createCommandPools() {
        // Graphics command pool
        VkCommandPoolCreateInfo graphicsPoolInfo{};
        graphicsPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        graphicsPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        graphicsPoolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

        VkResult result = vkCreateCommandPool(logicalDevice, &graphicsPoolInfo, nullptr, &graphicsCommandPool);
        if (result != VK_SUCCESS) {
            D6_THROW(VulkanException, Format("Failed to create graphics command pool, error: {0}") << result);
        }

        // Transfer command pool (might be the same as graphics)
        if (queueFamilyIndices.transferFamily.has_value() && 
            queueFamilyIndices.transferFamily.value() != queueFamilyIndices.graphicsFamily.value()) {
            
            VkCommandPoolCreateInfo transferPoolInfo{};
            transferPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            transferPoolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
            transferPoolInfo.queueFamilyIndex = queueFamilyIndices.transferFamily.value();

            result = vkCreateCommandPool(logicalDevice, &transferPoolInfo, nullptr, &transferCommandPool);
            if (result != VK_SUCCESS) {
                D6_THROW(VulkanException, Format("Failed to create transfer command pool, error: {0}") << result);
            }
        } else {
            transferCommandPool = graphicsCommandPool;
        }

        console->printLine("...Vulkan command pools created");
    }

    std::vector<const char*> VulkanDevice::getRequiredDeviceExtensions() const {
        return {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
            VK_KHR_MAINTENANCE1_EXTENSION_NAME  // Needed for negative viewport height (GL Y-flip)
        };
    }

    VkPhysicalDeviceProperties VulkanDevice::getDeviceProperties() const {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(physicalDevice, &properties);
        return properties;
    }

    VkPhysicalDeviceFeatures VulkanDevice::getDeviceFeatures() const {
        VkPhysicalDeviceFeatures features;
        vkGetPhysicalDeviceFeatures(physicalDevice, &features);
        return features;
    }

    VkPhysicalDeviceMemoryProperties VulkanDevice::getMemoryProperties() const {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
        return memProperties;
    }

    VkFormat VulkanDevice::findSupportedFormat(const std::vector<VkFormat>& candidates, 
                                               VkImageTiling tiling, 
                                               VkFormatFeatureFlags features) const {
        for (VkFormat format : candidates) {
            VkFormatProperties props;
            vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

            if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
                return format;
            } else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
                return format;
            }
        }

        D6_THROW(VulkanException, "Failed to find supported format");
    }

    uint32_t VulkanDevice::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const {
        VkPhysicalDeviceMemoryProperties memProperties = getMemoryProperties();

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if ((typeFilter & (1 << i)) && 
                (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }

        D6_THROW(VulkanException, "Failed to find suitable memory type");
    }

    VkCommandBuffer VulkanDevice::beginSingleTimeCommands() const {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = transferCommandPool;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(logicalDevice, &allocInfo, &commandBuffer);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        return commandBuffer;
    }

    void VulkanDevice::endSingleTimeCommands(VkCommandBuffer commandBuffer) const {
        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        vkQueueSubmit(transferQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(transferQueue);

        vkFreeCommandBuffers(logicalDevice, transferCommandPool, 1, &commandBuffer);
    }
}