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

#include "VulkanMemory.h"
#include "../../Format.h"
#include <cstring>

namespace Duel6 {
    VulkanMemoryManager::VulkanMemoryManager(Console& console)
        : console(&console) {
    }

    VulkanMemoryManager::~VulkanMemoryManager() {
    }

    void VulkanMemoryManager::initialize(const VulkanDevice& device, VkInstance instance) {
        console->printLine("...Vulkan memory manager initialized with manual allocation");
    }

    void VulkanMemoryManager::cleanup(const VulkanDevice& device) {
        // Nothing to cleanup for manual allocation
    }

    VulkanBuffer VulkanMemoryManager::createBuffer(const VulkanDevice& device, VkDeviceSize size, 
                                                   VkBufferUsageFlags usage, VkMemoryPropertyFlags properties) {
        VulkanBuffer buffer;
        buffer.size = size;

        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VkResult result = vkCreateBuffer(device.getLogicalDevice(), &bufferInfo, nullptr, &buffer.buffer);
        if (result != VK_SUCCESS) {
            D6_THROW(VulkanException, Format("Failed to create buffer, error: {0}") << result);
        }

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(device.getLogicalDevice(), buffer.buffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(device, memRequirements.memoryTypeBits, properties);

        result = vkAllocateMemory(device.getLogicalDevice(), &allocInfo, nullptr, &buffer.memory);
        if (result != VK_SUCCESS) {
            vkDestroyBuffer(device.getLogicalDevice(), buffer.buffer, nullptr);
            D6_THROW(VulkanException, Format("Failed to allocate buffer memory, error: {0}") << result);
        }

        vkBindBufferMemory(device.getLogicalDevice(), buffer.buffer, buffer.memory, 0);

        return buffer;
    }

    void VulkanMemoryManager::destroyBuffer(const VulkanDevice& device, VulkanBuffer& buffer) {
        if (buffer.mapped != nullptr) {
            unmapBuffer(device, buffer);
        }

        if (buffer.buffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(device.getLogicalDevice(), buffer.buffer, nullptr);
            buffer.buffer = VK_NULL_HANDLE;
        }

        if (buffer.memory != VK_NULL_HANDLE) {
            vkFreeMemory(device.getLogicalDevice(), buffer.memory, nullptr);
            buffer.memory = VK_NULL_HANDLE;
        }

        buffer.size = 0;
    }

    void* VulkanMemoryManager::mapBuffer(const VulkanDevice& device, VulkanBuffer& buffer) {
        if (buffer.mapped == nullptr) {
            VkResult result = vkMapMemory(device.getLogicalDevice(), buffer.memory, 0, buffer.size, 0, &buffer.mapped);
            if (result != VK_SUCCESS) {
                D6_THROW(VulkanException, Format("Failed to map buffer memory, error: {0}") << result);
            }
        }
        return buffer.mapped;
    }

    void VulkanMemoryManager::unmapBuffer(const VulkanDevice& device, VulkanBuffer& buffer) {
        if (buffer.mapped != nullptr) {
            vkUnmapMemory(device.getLogicalDevice(), buffer.memory);
            buffer.mapped = nullptr;
        }
    }

    void VulkanMemoryManager::copyBuffer(const VulkanDevice& device, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
        VkCommandBuffer commandBuffer = device.beginSingleTimeCommands();

        VkBufferCopy copyRegion{};
        copyRegion.size = size;
        vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

        device.endSingleTimeCommands(commandBuffer);
    }

    VulkanImage VulkanMemoryManager::createImage(const VulkanDevice& device, uint32_t width, uint32_t height,
                                                 VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
                                                 VkMemoryPropertyFlags properties) {
        VulkanImage image;
        image.format = format;
        image.extent = {width, height};

        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = width;
        imageInfo.extent.height = height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = format;
        imageInfo.tiling = tiling;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = usage;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VkResult result = vkCreateImage(device.getLogicalDevice(), &imageInfo, nullptr, &image.image);
        if (result != VK_SUCCESS) {
            D6_THROW(VulkanException, Format("Failed to create image, error: {0}") << result);
        }

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(device.getLogicalDevice(), image.image, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(device, memRequirements.memoryTypeBits, properties);

        result = vkAllocateMemory(device.getLogicalDevice(), &allocInfo, nullptr, &image.memory);
        if (result != VK_SUCCESS) {
            vkDestroyImage(device.getLogicalDevice(), image.image, nullptr);
            D6_THROW(VulkanException, Format("Failed to allocate image memory, error: {0}") << result);
        }

        vkBindImageMemory(device.getLogicalDevice(), image.image, image.memory, 0);

        // Create image view
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = image.image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = format;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        result = vkCreateImageView(device.getLogicalDevice(), &viewInfo, nullptr, &image.view);
        if (result != VK_SUCCESS) {
            vkFreeMemory(device.getLogicalDevice(), image.memory, nullptr);
            vkDestroyImage(device.getLogicalDevice(), image.image, nullptr);
            D6_THROW(VulkanException, Format("Failed to create image view, error: {0}") << result);
        }

        return image;
    }

    void VulkanMemoryManager::destroyImage(const VulkanDevice& device, VulkanImage& image) {
        if (image.view != VK_NULL_HANDLE) {
            vkDestroyImageView(device.getLogicalDevice(), image.view, nullptr);
            image.view = VK_NULL_HANDLE;
        }

        if (image.image != VK_NULL_HANDLE) {
            vkDestroyImage(device.getLogicalDevice(), image.image, nullptr);
            image.image = VK_NULL_HANDLE;
        }

        if (image.memory != VK_NULL_HANDLE) {
            vkFreeMemory(device.getLogicalDevice(), image.memory, nullptr);
            image.memory = VK_NULL_HANDLE;
        }

        image.format = VK_FORMAT_UNDEFINED;
        image.extent = {0, 0};
    }

    void VulkanMemoryManager::transitionImageLayout(const VulkanDevice& device, VkImage image, VkFormat format,
                                                    VkImageLayout oldLayout, VkImageLayout newLayout) {
        VkCommandBuffer commandBuffer = device.beginSingleTimeCommands();

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        VkPipelineStageFlags sourceStage;
        VkPipelineStageFlags destinationStage;

        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        } else {
            D6_THROW(VulkanException, "Unsupported layout transition");
        }

        vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

        device.endSingleTimeCommands(commandBuffer);
    }

    void VulkanMemoryManager::copyBufferToImage(const VulkanDevice& device, VkBuffer buffer, VkImage image,
                                                uint32_t width, uint32_t height) {
        VkCommandBuffer commandBuffer = device.beginSingleTimeCommands();

        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = {0, 0, 0};
        region.imageExtent = {width, height, 1};

        vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        device.endSingleTimeCommands(commandBuffer);
    }

    void VulkanMemoryManager::uploadBufferData(const VulkanDevice& device, VulkanBuffer& buffer, const void* data, VkDeviceSize size) {
        void* mappedData = mapBuffer(device, buffer);
        memcpy(mappedData, data, static_cast<size_t>(size));
        unmapBuffer(device, buffer);
    }

    VulkanBuffer VulkanMemoryManager::createStagingBuffer(const VulkanDevice& device, VkDeviceSize size) {
        return createBuffer(device, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                           VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    }

    uint32_t VulkanMemoryManager::findMemoryType(const VulkanDevice& device, uint32_t typeFilter, VkMemoryPropertyFlags properties) {
        return device.findMemoryType(typeFilter, properties);
    }
}