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

#include "VulkanBuffer.h"
#include "VulkanContext.h"
#include "../../Format.h"
#include <algorithm>
#include <cstring>

namespace Duel6 {
    // VulkanBufferBase implementation
    VulkanBufferBase::VulkanBufferBase(Console& console, BufferUsage usage)
        : usage(usage), capacity(0), currentSize(0), isPersistentlyMapped(false), console(&console) {
    }

    VulkanBufferBase::~VulkanBufferBase() {
        if (isValid()) {
            console->printLine("Warning: VulkanBuffer destroyed without explicit cleanup");
        }
    }

    VulkanBufferBase::VulkanBufferBase(VulkanBufferBase&& other) noexcept
        : buffer(std::move(other.buffer)), usage(other.usage), capacity(other.capacity),
          currentSize(other.currentSize), isPersistentlyMapped(other.isPersistentlyMapped), console(other.console) {
        other.capacity = 0;
        other.currentSize = 0;
        other.isPersistentlyMapped = false;
    }

    VulkanBufferBase& VulkanBufferBase::operator=(VulkanBufferBase&& other) noexcept {
        if (this != &other) {
            buffer = std::move(other.buffer);
            usage = other.usage;
            capacity = other.capacity;
            currentSize = other.currentSize;
            isPersistentlyMapped = other.isPersistentlyMapped;
            console = other.console;

            other.capacity = 0;
            other.currentSize = 0;
            other.isPersistentlyMapped = false;
        }
        return *this;
    }

    void VulkanBufferBase::create(const VulkanDevice& device, VulkanMemoryManager& memoryManager,
                                 VkDeviceSize size, bool persistentlyMapped) {
        if (isValid()) {
            destroy(device, memoryManager);
        }

        capacity = size;
        currentSize = 0;
        isPersistentlyMapped = persistentlyMapped;

        VkBufferUsageFlags vulkanUsage = getVulkanUsageFlags();
        VkMemoryPropertyFlags memProps = getMemoryPropertyFlags();

        buffer = memoryManager.createBuffer(device, capacity, vulkanUsage, memProps);

        if (isPersistentlyMapped) {
            memoryManager.mapBuffer(device, buffer);
        }
    }

    void VulkanBufferBase::destroy(const VulkanDevice& device, VulkanMemoryManager& memoryManager) {
        if (isValid()) {
            if (isPersistentlyMapped && buffer.mapped) {
                memoryManager.unmapBuffer(device, buffer);
            }
            memoryManager.destroyBuffer(device, buffer);
            capacity = 0;
            currentSize = 0;
            isPersistentlyMapped = false;
        }
    }

    void VulkanBufferBase::uploadData(const VulkanDevice& device, VulkanMemoryManager& memoryManager,
                                     const void* data, VkDeviceSize size, VkDeviceSize offset) {
        if (!isValid()) {
            D6_THROW(VulkanException, "Cannot upload to invalid buffer");
        }

        if (offset + size > capacity) {
            D6_THROW(VulkanException, "Data upload exceeds buffer capacity");
        }

        VkMemoryPropertyFlags memProps = getMemoryPropertyFlags();
        if (memProps & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
            // Direct upload for host-visible memory
            void* mapped = memoryManager.mapBuffer(device, buffer);
            memcpy(static_cast<char*>(mapped) + offset, data, static_cast<size_t>(size));
            
            if (!isPersistentlyMapped) {
                memoryManager.unmapBuffer(device, buffer);
            }
        } else {
            // Use staging for device-local memory
            uploadDataStaged(device, memoryManager, data, size, offset);
        }

        currentSize = std::max(currentSize, offset + size);
    }

    void VulkanBufferBase::uploadDataStaged(const VulkanDevice& device, VulkanMemoryManager& memoryManager,
                                           const void* data, VkDeviceSize size, VkDeviceSize offset) {
        // Create staging buffer
        VulkanBuffer stagingBuffer = memoryManager.createStagingBuffer(device, size);

        // Upload to staging buffer
        memoryManager.uploadBufferData(device, stagingBuffer, data, size);

        // Copy from staging to target buffer
        VkCommandBuffer commandBuffer = device.beginSingleTimeCommands();

        VkBufferCopy copyRegion{};
        copyRegion.srcOffset = 0;
        copyRegion.dstOffset = offset;
        copyRegion.size = size;

        vkCmdCopyBuffer(commandBuffer, stagingBuffer.buffer, buffer.buffer, 1, &copyRegion);

        device.endSingleTimeCommands(commandBuffer);

        // Cleanup staging buffer
        memoryManager.destroyBuffer(device, stagingBuffer);
    }

    void* VulkanBufferBase::map(const VulkanDevice& device, VulkanMemoryManager& memoryManager) {
        if (!isValid()) {
            D6_THROW(VulkanException, "Cannot map invalid buffer");
        }

        return memoryManager.mapBuffer(device, buffer);
    }

    void VulkanBufferBase::unmap(const VulkanDevice& device, VulkanMemoryManager& memoryManager) {
        if (isValid() && !isPersistentlyMapped) {
            memoryManager.unmapBuffer(device, buffer);
        }
    }

    void VulkanBufferBase::flush(const VulkanDevice& device, VulkanMemoryManager& memoryManager,
                                VkDeviceSize offset, VkDeviceSize size) {
        if (!isValid() || !buffer.mapped) {
            return;
        }

        VkDeviceSize flushSize = (size == VK_WHOLE_SIZE) ? capacity - offset : size;
        
        VkMappedMemoryRange range{};
        range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        range.memory = VK_NULL_HANDLE; // Will need to get this from VMA if needed
        range.offset = offset;
        range.size = flushSize;

        // For VMA with HOST_COHERENT memory, explicit flushing is usually not needed
        // This is a placeholder for cases where manual flushing might be required
    }

    VkMemoryPropertyFlags VulkanBufferBase::getMemoryPropertyFlags() const {
        return VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    }

    // VulkanVertexBuffer implementation
    VulkanVertexBuffer::VulkanVertexBuffer(Console& console)
        : VulkanBufferBase(console, BufferUsage::Vertex) {
    }

    // VulkanIndexBuffer implementation
    VulkanIndexBuffer::VulkanIndexBuffer(Console& console, VkIndexType type)
        : VulkanBufferBase(console, BufferUsage::Index), indexType(type) {
    }

    void VulkanIndexBuffer::uploadIndices16(const VulkanDevice& device, VulkanMemoryManager& memoryManager,
                                           const std::vector<uint16_t>& indices) {
        indexType = VK_INDEX_TYPE_UINT16;
        VkDeviceSize dataSize = indices.size() * sizeof(uint16_t);
        
        if (dataSize > capacity) {
            destroy(device, memoryManager);
            create(device, memoryManager, dataSize * 2);
        }
        
        uploadData(device, memoryManager, indices.data(), dataSize);
        currentSize = dataSize;
    }

    void VulkanIndexBuffer::uploadIndices32(const VulkanDevice& device, VulkanMemoryManager& memoryManager,
                                           const std::vector<uint32_t>& indices) {
        indexType = VK_INDEX_TYPE_UINT32;
        VkDeviceSize dataSize = indices.size() * sizeof(uint32_t);
        
        if (dataSize > capacity) {
            destroy(device, memoryManager);
            create(device, memoryManager, dataSize * 2);
        }
        
        uploadData(device, memoryManager, indices.data(), dataSize);
        currentSize = dataSize;
    }

    uint32_t VulkanIndexBuffer::getIndexCount() const {
        size_t indexSize = (indexType == VK_INDEX_TYPE_UINT16) ? sizeof(uint16_t) : sizeof(uint32_t);
        return static_cast<uint32_t>(currentSize / indexSize);
    }

    // VulkanUniformBuffer implementation
    VulkanUniformBuffer::VulkanUniformBuffer(Console& console)
        : VulkanBufferBase(console, BufferUsage::Uniform), alignment(0) {
    }

    void VulkanUniformBuffer::create(const VulkanDevice& device, VulkanMemoryManager& memoryManager,
                                    VkDeviceSize size, bool persistentlyMapped) {
        // Get minimum uniform buffer offset alignment
        VkPhysicalDeviceProperties properties = device.getDeviceProperties();
        alignment = properties.limits.minUniformBufferOffsetAlignment;

        // Align the buffer size
        VkDeviceSize alignedSize = alignUp(size, alignment);

        VulkanBufferBase::create(device, memoryManager, alignedSize, persistentlyMapped);
    }

    // VulkanStagingBuffer implementation
    VulkanStagingBuffer::VulkanStagingBuffer(Console& console)
        : VulkanBufferBase(console, BufferUsage::Staging) {
    }

    void VulkanStagingBuffer::copyToBuffer(const VulkanDevice& device, VulkanBufferBase& dstBuffer,
                                          VkDeviceSize srcOffset, VkDeviceSize dstOffset, VkDeviceSize size) {
        if (!isValid() || !dstBuffer.isValid()) {
            D6_THROW(VulkanException, "Cannot copy between invalid buffers");
        }

        VkDeviceSize copySize = (size == VK_WHOLE_SIZE) ? 
            std::min(currentSize - srcOffset, dstBuffer.getCapacity() - dstOffset) : size;

        VkCommandBuffer commandBuffer = device.beginSingleTimeCommands();

        VkBufferCopy copyRegion{};
        copyRegion.srcOffset = srcOffset;
        copyRegion.dstOffset = dstOffset;
        copyRegion.size = copySize;

        vkCmdCopyBuffer(commandBuffer, buffer.buffer, dstBuffer.getHandle(), 1, &copyRegion);

        device.endSingleTimeCommands(commandBuffer);
    }

    void VulkanStagingBuffer::copyToImage(const VulkanDevice& device, VkImage dstImage, uint32_t width, uint32_t height,
                                         VkDeviceSize offset) {
        if (!isValid()) {
            D6_THROW(VulkanException, "Cannot copy from invalid staging buffer");
        }

        VkCommandBuffer commandBuffer = device.beginSingleTimeCommands();

        VkBufferImageCopy region{};
        region.bufferOffset = offset;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = {0, 0, 0};
        region.imageExtent = {width, height, 1};

        vkCmdCopyBufferToImage(commandBuffer, buffer.buffer, dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        device.endSingleTimeCommands(commandBuffer);
    }
}