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

#ifndef DUEL6_RENDERER_VULKANBUFFER_H
#define DUEL6_RENDERER_VULKANBUFFER_H

#include <vulkan/vulkan.h>
#include <vector>
#include <memory>
#include "VulkanMemory.h"
#include "VulkanDevice.h"
#include "../../Type.h"
#include "../../console/Console.h"

namespace Duel6 {
    enum class BufferUsage {
        Vertex,
        Index,
        Uniform,
        Storage,
        Staging
    };

    class VulkanBufferBase {
    protected:
        VulkanBuffer buffer;
        BufferUsage usage;
        VkDeviceSize capacity;
        VkDeviceSize currentSize;
        bool isPersistentlyMapped;
        Console* console;

    public:
        explicit VulkanBufferBase(Console& console, BufferUsage usage);
        virtual ~VulkanBufferBase();

        // Non-copyable but moveable
        VulkanBufferBase(const VulkanBufferBase&) = delete;
        VulkanBufferBase& operator=(const VulkanBufferBase&) = delete;
        VulkanBufferBase(VulkanBufferBase&& other) noexcept;
        VulkanBufferBase& operator=(VulkanBufferBase&& other) noexcept;

        virtual void create(const VulkanDevice& device, VulkanMemoryManager& memoryManager, 
                           VkDeviceSize size, bool persistentlyMapped = false);
        virtual void destroy(const VulkanDevice& device, VulkanMemoryManager& memoryManager);

        // Data upload methods
        void uploadData(const VulkanDevice& device, VulkanMemoryManager& memoryManager, 
                       const void* data, VkDeviceSize size, VkDeviceSize offset = 0);
        void uploadDataStaged(const VulkanDevice& device, VulkanMemoryManager& memoryManager,
                             const void* data, VkDeviceSize size, VkDeviceSize offset = 0);

        // Memory mapping
        void* map(const VulkanDevice& device, VulkanMemoryManager& memoryManager);
        void unmap(const VulkanDevice& device, VulkanMemoryManager& memoryManager);
        void flush(const VulkanDevice& device, VulkanMemoryManager& memoryManager, 
                  VkDeviceSize offset = 0, VkDeviceSize size = VK_WHOLE_SIZE);

        // Getters
        VkBuffer getHandle() const { return buffer.buffer; }
        const VulkanBuffer& getBuffer() const { return buffer; }
        BufferUsage getUsage() const { return usage; }
        VkDeviceSize getCapacity() const { return capacity; }
        VkDeviceSize getCurrentSize() const { return currentSize; }
        bool isValid() const { return buffer.buffer != VK_NULL_HANDLE; }
        bool isMapped() const { return buffer.mapped != nullptr; }

    protected:
        virtual VkBufferUsageFlags getVulkanUsageFlags() const = 0;
        virtual VkMemoryPropertyFlags getMemoryPropertyFlags() const;
    };

    class VulkanVertexBuffer : public VulkanBufferBase {
    public:
        explicit VulkanVertexBuffer(Console& console);

        template<typename VertexType>
        void uploadVertices(const VulkanDevice& device, VulkanMemoryManager& memoryManager,
                           const std::vector<VertexType>& vertices) {
            VkDeviceSize dataSize = vertices.size() * sizeof(VertexType);
            if (dataSize > capacity) {
                // Recreate buffer with larger size
                destroy(device, memoryManager);
                create(device, memoryManager, dataSize * 2); // Allocate extra space for growth
            }
            uploadData(device, memoryManager, vertices.data(), dataSize);
            currentSize = dataSize;
        }

        uint32_t getVertexCount(size_t vertexSize) const {
            return static_cast<uint32_t>(currentSize / vertexSize);
        }

    protected:
        VkBufferUsageFlags getVulkanUsageFlags() const override {
            return VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        }
    };

    class VulkanIndexBuffer : public VulkanBufferBase {
    private:
        VkIndexType indexType;

    public:
        explicit VulkanIndexBuffer(Console& console, VkIndexType type = VK_INDEX_TYPE_UINT32);

        void uploadIndices16(const VulkanDevice& device, VulkanMemoryManager& memoryManager,
                            const std::vector<uint16_t>& indices);
        void uploadIndices32(const VulkanDevice& device, VulkanMemoryManager& memoryManager,
                            const std::vector<uint32_t>& indices);

        VkIndexType getIndexType() const { return indexType; }
        uint32_t getIndexCount() const;

    protected:
        VkBufferUsageFlags getVulkanUsageFlags() const override {
            return VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        }
    };

    class VulkanUniformBuffer : public VulkanBufferBase {
    private:
        VkDeviceSize alignment;

    public:
        explicit VulkanUniformBuffer(Console& console);

        void create(const VulkanDevice& device, VulkanMemoryManager& memoryManager, 
                   VkDeviceSize size, bool persistentlyMapped = true) override;

        template<typename UniformType>
        void uploadUniform(const VulkanDevice& device, VulkanMemoryManager& memoryManager,
                          const UniformType& uniform, VkDeviceSize offset = 0) {
            VkDeviceSize alignedOffset = alignUp(offset, alignment);
            uploadData(device, memoryManager, &uniform, sizeof(UniformType), alignedOffset);
        }

        VkDeviceSize getAlignment() const { return alignment; }
        VkDeviceSize alignUp(VkDeviceSize size, VkDeviceSize alignment) const {
            return (size + alignment - 1) & ~(alignment - 1);
        }

    protected:
        VkBufferUsageFlags getVulkanUsageFlags() const override {
            return VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        }

        VkMemoryPropertyFlags getMemoryPropertyFlags() const override {
            return VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        }
    };

    class VulkanStagingBuffer : public VulkanBufferBase {
    public:
        explicit VulkanStagingBuffer(Console& console);

        void copyToBuffer(const VulkanDevice& device, VulkanBufferBase& dstBuffer,
                         VkDeviceSize srcOffset = 0, VkDeviceSize dstOffset = 0, VkDeviceSize size = VK_WHOLE_SIZE);
        void copyToImage(const VulkanDevice& device, VkImage dstImage, uint32_t width, uint32_t height,
                        VkDeviceSize offset = 0);

    protected:
        VkBufferUsageFlags getVulkanUsageFlags() const override {
            return VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        }

        VkMemoryPropertyFlags getMemoryPropertyFlags() const override {
            return VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        }
    };

    // Convenience types for resource management
    using VertexBufferPtr = std::unique_ptr<VulkanVertexBuffer>;
    using IndexBufferPtr = std::unique_ptr<VulkanIndexBuffer>;
    using UniformBufferPtr = std::unique_ptr<VulkanUniformBuffer>;
    using StagingBufferPtr = std::unique_ptr<VulkanStagingBuffer>;
}

#endif