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

#include "VulkanResourceManager.h"
#include "VulkanContext.h"
#include "../../Format.h"
#include <algorithm>

namespace Duel6 {
    VulkanResourceManager::VulkanResourceManager(Console& console)
        : memoryManager(nullptr), samplerManager(nullptr), descriptorManager(nullptr), console(&console),
          nextTextureId(1), nextBufferId(1), totalTextureMemory(0), totalBufferMemory(0),
          peakTextureMemory(0), peakBufferMemory(0) {
    }

    VulkanResourceManager::~VulkanResourceManager() {
        if (!textures.empty() || !buffers.empty()) {
            console->printLine("Warning: VulkanResourceManager destroyed with resources still allocated");
        }
    }

    void VulkanResourceManager::initialize(VulkanMemoryManager* memMgr, VulkanSamplerManager* samplerMgr, VulkanDescriptorManager* descMgr) {
        memoryManager = memMgr;
        samplerManager = samplerMgr;
        descriptorManager = descMgr;
        console->printLine("...Vulkan resource manager initialized");
    }

    void VulkanResourceManager::cleanup(const VulkanDevice& device) {
        // Clean up all remaining textures
        for (auto& pair : textures) {
            destroyTextureResource(device, pair.second);
        }
        textures.clear();

        // Clean up all remaining buffers
        for (auto& pair : buffers) {
            destroyBufferResource(device, pair.second);
        }
        buffers.clear();

        // Clear deletion queues
        pendingTextureDeletes.clear();
        pendingBufferDeletes.clear();

        // Clear available ID queues
        while (!availableTextureIds.empty()) availableTextureIds.pop();
        while (!availableBufferIds.empty()) availableBufferIds.pop();

        nextTextureId = 1;
        nextBufferId = 1;
        totalTextureMemory = 0;
        totalBufferMemory = 0;

        console->printLine("...Vulkan resource manager cleaned up");
    }

    TextureId VulkanResourceManager::createTexture(const VulkanDevice& device, const Image& image,
                                                  TextureFilter filter, bool clamp, const std::string& debugName) {
        auto texture = std::make_unique<VulkanTexture>(*console);
        texture->create(device, *memoryManager, image, filter, clamp);

        // Create descriptor set
        VkDescriptorSetLayout layout = descriptorManager->getTextureLayout(device);
        VkDescriptorSet descriptorSet = descriptorManager->allocateDescriptorSet(device, layout);
        
        if (descriptorSet != VK_NULL_HANDLE) {
            VkSampler sampler = samplerManager->getSampler(device, filter, clamp);
            descriptorManager->updateTextureDescriptorSet(device, descriptorSet, 0, 
                                                         texture->getImage().view, sampler);
            texture->setDescriptorSet(descriptorSet);
        }

        TextureId id = getNextTextureId();
        TextureResource resource;
        resource.texture = std::move(texture);
        resource.descriptorSet = descriptorSet;
        resource.state = ResourceState::Created;
        resource.refCount = 1;
        resource.debugName = debugName;

        textures[id] = std::move(resource);
        updateMemoryStats();

        return id;
    }

    TextureId VulkanResourceManager::createTexture(const VulkanDevice& device, uint32_t width, uint32_t height,
                                                  VkFormat format, TextureFilter filter, bool clamp, const std::string& debugName) {
        auto texture = std::make_unique<VulkanTexture>(*console);
        texture->create(device, *memoryManager, width, height, format, filter, clamp);

        // Create descriptor set
        VkDescriptorSetLayout layout = descriptorManager->getTextureLayout(device);
        VkDescriptorSet descriptorSet = descriptorManager->allocateDescriptorSet(device, layout);
        
        if (descriptorSet != VK_NULL_HANDLE) {
            VkSampler sampler = samplerManager->getSampler(device, filter, clamp);
            descriptorManager->updateTextureDescriptorSet(device, descriptorSet, 0,
                                                         texture->getImage().view, sampler);
            texture->setDescriptorSet(descriptorSet);
        }

        TextureId id = getNextTextureId();
        TextureResource resource;
        resource.texture = std::move(texture);
        resource.descriptorSet = descriptorSet;
        resource.state = ResourceState::Created;
        resource.refCount = 1;
        resource.debugName = debugName;

        textures[id] = std::move(resource);
        updateMemoryStats();

        return id;
    }

    void VulkanResourceManager::updateTexture(const VulkanDevice& device, TextureId id, const Image& image) {
        auto it = textures.find(id);
        if (it != textures.end() && it->second.state != ResourceState::Deleted) {
            it->second.texture->updateData(device, *memoryManager, image);
        }
    }

    void VulkanResourceManager::deleteTexture(TextureId id) {
        auto it = textures.find(id);
        if (it != textures.end() && it->second.state != ResourceState::Deleted) {
            it->second.state = ResourceState::PendingDeletion;
            pendingTextureDeletes.push_back(id);
        }
    }

    VulkanTexture* VulkanResourceManager::getTexture(TextureId id) {
        auto it = textures.find(id);
        if (it != textures.end() && it->second.state != ResourceState::Deleted) {
            return it->second.texture.get();
        }
        return nullptr;
    }

    const VulkanTexture* VulkanResourceManager::getTexture(TextureId id) const {
        auto it = textures.find(id);
        if (it != textures.end() && it->second.state != ResourceState::Deleted) {
            return it->second.texture.get();
        }
        return nullptr;
    }

    VkDescriptorSet VulkanResourceManager::getTextureDescriptorSet(TextureId id) {
        auto it = textures.find(id);
        if (it != textures.end() && it->second.state != ResourceState::Deleted) {
            return it->second.descriptorSet;
        }
        return VK_NULL_HANDLE;
    }

    void VulkanResourceManager::deleteBuffer(BufferId id) {
        auto it = buffers.find(id);
        if (it != buffers.end() && it->second.state != ResourceState::Deleted) {
            it->second.state = ResourceState::PendingDeletion;
            pendingBufferDeletes.push_back(id);
        }
    }

    void VulkanResourceManager::addTextureRef(TextureId id) {
        auto it = textures.find(id);
        if (it != textures.end() && it->second.state != ResourceState::Deleted) {
            it->second.refCount++;
        }
    }

    void VulkanResourceManager::releaseTextureRef(TextureId id) {
        auto it = textures.find(id);
        if (it != textures.end() && it->second.state != ResourceState::Deleted) {
            if (--it->second.refCount == 0) {
                deleteTexture(id);
            }
        }
    }

    void VulkanResourceManager::addBufferRef(BufferId id) {
        auto it = buffers.find(id);
        if (it != buffers.end() && it->second.state != ResourceState::Deleted) {
            it->second.refCount++;
        }
    }

    void VulkanResourceManager::releaseBufferRef(BufferId id) {
        auto it = buffers.find(id);
        if (it != buffers.end() && it->second.state != ResourceState::Deleted) {
            if (--it->second.refCount == 0) {
                deleteBuffer(id);
            }
        }
    }

    void VulkanResourceManager::processDeletes(const VulkanDevice& device) {
        // Process texture deletes
        for (TextureId id : pendingTextureDeletes) {
            auto it = textures.find(id);
            if (it != textures.end()) {
                destroyTextureResource(device, it->second);
                recycleTextureId(id);
                textures.erase(it);
            }
        }
        pendingTextureDeletes.clear();

        // Process buffer deletes
        for (BufferId id : pendingBufferDeletes) {
            auto it = buffers.find(id);
            if (it != buffers.end()) {
                destroyBufferResource(device, it->second);
                recycleBufferId(id);
                buffers.erase(it);
            }
        }
        pendingBufferDeletes.clear();

        updateMemoryStats();
    }

    bool VulkanResourceManager::isValidTexture(TextureId id) const {
        auto it = textures.find(id);
        return it != textures.end() && it->second.state != ResourceState::Deleted;
    }

    bool VulkanResourceManager::isValidBuffer(BufferId id) const {
        auto it = buffers.find(id);
        return it != buffers.end() && it->second.state != ResourceState::Deleted;
    }

    void VulkanResourceManager::updateMemoryStats() {
        totalTextureMemory = 0;
        totalBufferMemory = 0;

        for (const auto& pair : textures) {
            if (pair.second.state != ResourceState::Deleted) {
                totalTextureMemory += estimateTextureMemory(pair.second.texture.get());
            }
        }

        for (const auto& pair : buffers) {
            if (pair.second.state != ResourceState::Deleted) {
                totalBufferMemory += estimateBufferMemory(pair.second.buffer.get());
            }
        }

        peakTextureMemory = std::max(peakTextureMemory, totalTextureMemory);
        peakBufferMemory = std::max(peakBufferMemory, totalBufferMemory);
    }

    void VulkanResourceManager::dumpResourceStats() const {
        console->printLine(Format("Vulkan Resource Stats:"));
        console->printLine(Format("  Textures: {0} ({1} KB)") << textures.size() << (totalTextureMemory / 1024));
        console->printLine(Format("  Buffers: {0} ({1} KB)") << buffers.size() << (totalBufferMemory / 1024));
        console->printLine(Format("  Peak Texture Memory: {0} KB") << (peakTextureMemory / 1024));
        console->printLine(Format("  Peak Buffer Memory: {0} KB") << (peakBufferMemory / 1024));
        console->printLine(Format("  Pending Deletes: {0} textures, {1} buffers") 
                          << pendingTextureDeletes.size() << pendingBufferDeletes.size());
    }

    void VulkanResourceManager::setResourceDebugName(uint32_t id, const std::string& name) {
        // Try texture first
        auto texIt = textures.find(id);
        if (texIt != textures.end()) {
            texIt->second.debugName = name;
            return;
        }
        
        // Try buffer
        auto bufIt = buffers.find(id);
        if (bufIt != buffers.end()) {
            bufIt->second.debugName = name;
        }
    }

    TextureId VulkanResourceManager::getNextTextureId() {
        if (!availableTextureIds.empty()) {
            TextureId id = availableTextureIds.front();
            availableTextureIds.pop();
            return id;
        }
        return nextTextureId++;
    }

    BufferId VulkanResourceManager::getNextBufferId() {
        if (!availableBufferIds.empty()) {
            BufferId id = availableBufferIds.front();
            availableBufferIds.pop();
            return id;
        }
        return nextBufferId++;
    }

    void VulkanResourceManager::recycleTextureId(TextureId id) {
        availableTextureIds.push(id);
    }

    void VulkanResourceManager::recycleBufferId(BufferId id) {
        availableBufferIds.push(id);
    }

    void VulkanResourceManager::destroyTextureResource(const VulkanDevice& device, TextureResource& resource) {
        if (resource.texture) {
            resource.texture->destroy(device, *memoryManager);
        }
        resource.state = ResourceState::Deleted;
    }

    void VulkanResourceManager::destroyBufferResource(const VulkanDevice& device, BufferResource& resource) {
        if (resource.buffer) {
            resource.buffer->destroy(device, *memoryManager);
        }
        resource.state = ResourceState::Deleted;
    }

    size_t VulkanResourceManager::estimateTextureMemory(const VulkanTexture* texture) const {
        if (!texture || !texture->valid()) {
            return 0;
        }

        // Rough estimate: width * height * 4 bytes (RGBA)
        return texture->getWidth() * texture->getHeight() * 4;
    }

    size_t VulkanResourceManager::estimateBufferMemory(const VulkanBufferBase* buffer) const {
        if (!buffer || !buffer->isValid()) {
            return 0;
        }

        return static_cast<size_t>(buffer->getCapacity());
    }
}