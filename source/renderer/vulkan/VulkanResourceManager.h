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

#ifndef DUEL6_RENDERER_VULKANRESOURCEMANAGER_H
#define DUEL6_RENDERER_VULKANRESOURCEMANAGER_H

#include <vulkan/vulkan.h>
#include <memory>
#include <vector>
#include <unordered_map>
#include <queue>
#include "VulkanDevice.h"
#include "VulkanMemory.h"
#include "VulkanTexture.h"
#include "VulkanBuffer.h"
#include "VulkanSampler.h"
#include "VulkanDescriptor.h"
#include "VulkanTypes.h"
#include "../../Image.h"
#include "../../console/Console.h"

namespace Duel6 {
    // Resource handle types are defined in VulkanTypes.h

    // Resource state tracking
    enum class ResourceState {
        Created,
        InUse,
        PendingDeletion,
        Deleted
    };

    struct TextureResource {
        std::unique_ptr<VulkanTexture> texture;
        VkDescriptorSet descriptorSet;
        ResourceState state;
        uint32_t refCount;
        std::string debugName;

        TextureResource() : descriptorSet(VK_NULL_HANDLE), state(ResourceState::Created), refCount(0) {}
    };

    struct BufferResource {
        std::unique_ptr<VulkanBufferBase> buffer;
        ResourceState state;
        uint32_t refCount;
        std::string debugName;

        BufferResource() : state(ResourceState::Created), refCount(0) {}
    };

    class VulkanResourceManager {
    private:
        VulkanMemoryManager* memoryManager;
        VulkanSamplerManager* samplerManager;
        VulkanDescriptorManager* descriptorManager;
        Console* console;

        // Resource storage
        std::unordered_map<TextureId, TextureResource> textures;
        std::unordered_map<BufferId, BufferResource> buffers;
        
        // ID generation
        TextureId nextTextureId;
        BufferId nextBufferId;
        std::queue<TextureId> availableTextureIds;
        std::queue<BufferId> availableBufferIds;

        // Deletion queues for deferred cleanup
        std::vector<TextureId> pendingTextureDeletes;
        std::vector<BufferId> pendingBufferDeletes;

        // Statistics
        size_t totalTextureMemory;
        size_t totalBufferMemory;
        size_t peakTextureMemory;
        size_t peakBufferMemory;

    public:
        explicit VulkanResourceManager(Console& console);
        ~VulkanResourceManager();

        // Non-copyable
        VulkanResourceManager(const VulkanResourceManager&) = delete;
        VulkanResourceManager& operator=(const VulkanResourceManager&) = delete;

        void initialize(VulkanMemoryManager* memMgr, VulkanSamplerManager* samplerMgr, VulkanDescriptorManager* descMgr);
        void cleanup(const VulkanDevice& device);

        // Texture management
        TextureId createTexture(const VulkanDevice& device, const Image& image, 
                               TextureFilter filter, bool clamp, const std::string& debugName = "");
        TextureId createTexture(const VulkanDevice& device, uint32_t width, uint32_t height, 
                               VkFormat format, TextureFilter filter, bool clamp, const std::string& debugName = "");
        void updateTexture(const VulkanDevice& device, TextureId id, const Image& image);
        void deleteTexture(TextureId id);
        
        VulkanTexture* getTexture(TextureId id);
        const VulkanTexture* getTexture(TextureId id) const;
        VkDescriptorSet getTextureDescriptorSet(TextureId id);

        // Buffer management
        template<typename BufferType, typename... Args>
        BufferId createBuffer(const VulkanDevice& device, const std::string& debugName, Args&&... args) {
            auto buffer = std::make_unique<BufferType>(*console, std::forward<Args>(args)...);
            BufferId id = getNextBufferId();
            
            BufferResource resource;
            resource.buffer = std::move(buffer);
            resource.state = ResourceState::Created;
            resource.refCount = 1;
            resource.debugName = debugName;

            buffers[id] = std::move(resource);
            return id;
        }

        template<typename BufferType>
        BufferType* getBuffer(BufferId id) {
            auto it = buffers.find(id);
            if (it != buffers.end() && it->second.state != ResourceState::Deleted) {
                return static_cast<BufferType*>(it->second.buffer.get());
            }
            return nullptr;
        }

        void deleteBuffer(BufferId id);

        // Resource reference counting
        void addTextureRef(TextureId id);
        void releaseTextureRef(TextureId id);
        void addBufferRef(BufferId id);
        void releaseBufferRef(BufferId id);

        // Deferred cleanup (call per frame)
        void processDeletes(const VulkanDevice& device);

        // Resource queries
        bool isValidTexture(TextureId id) const;
        bool isValidBuffer(BufferId id) const;
        size_t getTextureCount() const { return textures.size(); }
        size_t getBufferCount() const { return buffers.size(); }

        // Memory statistics
        void updateMemoryStats();
        size_t getTotalTextureMemory() const { return totalTextureMemory; }
        size_t getTotalBufferMemory() const { return totalBufferMemory; }
        size_t getPeakTextureMemory() const { return peakTextureMemory; }
        size_t getPeakBufferMemory() const { return peakBufferMemory; }

        // Debug and diagnostics
        void dumpResourceStats() const;
        void setResourceDebugName(uint32_t id, const std::string& name);

    private:
        TextureId getNextTextureId();
        BufferId getNextBufferId();
        void recycleTextureId(TextureId id);
        void recycleBufferId(BufferId id);

        void destroyTextureResource(const VulkanDevice& device, TextureResource& resource);
        void destroyBufferResource(const VulkanDevice& device, BufferResource& resource);

        size_t estimateTextureMemory(const VulkanTexture* texture) const;
        size_t estimateBufferMemory(const VulkanBufferBase* buffer) const;
    };
}

#endif