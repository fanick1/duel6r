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

#ifndef DUEL6_RENDERER_VULKANMEMORY_H
#define DUEL6_RENDERER_VULKANMEMORY_H

#include <vulkan/vulkan.h>
#include <vector>
#include "VulkanDevice.h"
#include "../../console/Console.h"

namespace Duel6 {
    struct VulkanBuffer {
        VkBuffer buffer;
        VkDeviceMemory memory;
        VkDeviceSize size;
        void* mapped;

        VulkanBuffer() : buffer(VK_NULL_HANDLE), memory(VK_NULL_HANDLE), size(0), mapped(nullptr) {}
    };

    struct VulkanImage {
        VkImage image;
        VkDeviceMemory memory;
        VkImageView view;
        VkFormat format;
        VkExtent2D extent;
        uint32_t arrayLayers;

        VulkanImage() : image(VK_NULL_HANDLE), memory(VK_NULL_HANDLE), view(VK_NULL_HANDLE), 
                       format(VK_FORMAT_UNDEFINED), extent({0, 0}), arrayLayers(1) {}
    };

    class VulkanMemoryManager {
    private:
        Console* console;
        std::vector<VulkanBuffer> allocatedBuffers;
        std::vector<VulkanImage> allocatedImages;

    public:
        explicit VulkanMemoryManager(Console& console);
        ~VulkanMemoryManager();

        // Non-copyable
        VulkanMemoryManager(const VulkanMemoryManager&) = delete;
        VulkanMemoryManager& operator=(const VulkanMemoryManager&) = delete;

        void initialize(const VulkanDevice& device, VkInstance instance);
        void cleanup(const VulkanDevice& device);

        // Buffer management
        VulkanBuffer createBuffer(const VulkanDevice& device, VkDeviceSize size, 
                                  VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
        void destroyBuffer(const VulkanDevice& device, VulkanBuffer& buffer);
        void* mapBuffer(const VulkanDevice& device, VulkanBuffer& buffer);
        void unmapBuffer(const VulkanDevice& device, VulkanBuffer& buffer);
        void copyBuffer(const VulkanDevice& device, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

        // Image management
        VulkanImage createImage(const VulkanDevice& device, uint32_t width, uint32_t height, 
                                VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, 
                                VkMemoryPropertyFlags properties, uint32_t arrayLayers = 1);
        void destroyImage(const VulkanDevice& device, VulkanImage& image);
        void transitionImageLayout(const VulkanDevice& device, VkImage image, VkFormat format, 
                                   VkImageLayout oldLayout, VkImageLayout newLayout,
                                   uint32_t arrayLayers = 1);
        void copyBufferToImage(const VulkanDevice& device, VkBuffer buffer, VkImage image, 
                               uint32_t width, uint32_t height, uint32_t arrayLayers = 1);

        // Staging operations
        void uploadBufferData(const VulkanDevice& device, VulkanBuffer& buffer, const void* data, VkDeviceSize size);
        VulkanBuffer createStagingBuffer(const VulkanDevice& device, VkDeviceSize size);

    private:
        uint32_t findMemoryType(const VulkanDevice& device, uint32_t typeFilter, VkMemoryPropertyFlags properties);
    };
}

#endif