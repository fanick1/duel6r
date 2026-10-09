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

#ifndef DUEL6_RENDERER_VULKANDESCRIPTOR_H
#define DUEL6_RENDERER_VULKANDESCRIPTOR_H

#include <vulkan/vulkan.h>
#include <vector>
#include <unordered_map>
#include <memory>
#include "VulkanDevice.h"
#include "../../console/Console.h"

namespace Duel6 {
    class VulkanTexture;

    struct DescriptorSetLayoutKey {
        std::vector<VkDescriptorSetLayoutBinding> bindings;

        bool operator==(const DescriptorSetLayoutKey& other) const {
            if (bindings.size() != other.bindings.size()) {
                return false;
            }
            for (size_t i = 0; i < bindings.size(); ++i) {
                const auto& a = bindings[i];
                const auto& b = other.bindings[i];
                if (a.binding != b.binding || a.descriptorType != b.descriptorType ||
                    a.descriptorCount != b.descriptorCount || a.stageFlags != b.stageFlags) {
                    return false;
                }
            }
            return true;
        }
    };

    struct DescriptorSetLayoutKeyHash {
        std::size_t operator()(const DescriptorSetLayoutKey& k) const {
            std::size_t hash = 0;
            for (const auto& binding : k.bindings) {
                hash ^= std::hash<uint32_t>()(binding.binding) +
                        std::hash<uint32_t>()(binding.descriptorType) +
                        std::hash<uint32_t>()(binding.descriptorCount) +
                        std::hash<uint32_t>()(binding.stageFlags);
            }
            return hash;
        }
    };

    class VulkanDescriptorPool {
    private:
        VkDescriptorPool pool;
        std::vector<VkDescriptorPoolSize> poolSizes;
        uint32_t maxSets;
        uint32_t allocatedSets;
        Console* console;

    public:
        explicit VulkanDescriptorPool(Console& console);
        ~VulkanDescriptorPool();

        // Non-copyable
        VulkanDescriptorPool(const VulkanDescriptorPool&) = delete;
        VulkanDescriptorPool& operator=(const VulkanDescriptorPool&) = delete;

        void create(const VulkanDevice& device, uint32_t maxSets, const std::vector<VkDescriptorPoolSize>& poolSizes);
        void destroy(const VulkanDevice& device);
        void reset(const VulkanDevice& device);

        VkDescriptorSet allocateDescriptorSet(const VulkanDevice& device, VkDescriptorSetLayout layout);
        void freeDescriptorSet(const VulkanDevice& device, VkDescriptorSet descriptorSet);
        bool canAllocate() const { return allocatedSets < maxSets; }
        
        VkDescriptorPool getHandle() const { return pool; }
    };

    class VulkanDescriptorManager {
    private:
        std::unordered_map<DescriptorSetLayoutKey, VkDescriptorSetLayout, DescriptorSetLayoutKeyHash> layoutCache;
        std::vector<std::unique_ptr<VulkanDescriptorPool>> descriptorPools;
        std::unordered_map<VkDescriptorSet, VulkanDescriptorPool*> descriptorSetOwners;
        size_t currentPoolIndex;
        uint32_t maxSetsPerPool;
        Console* console;

    public:
        explicit VulkanDescriptorManager(Console& console);
        ~VulkanDescriptorManager();

        // Non-copyable
        VulkanDescriptorManager(const VulkanDescriptorManager&) = delete;
        VulkanDescriptorManager& operator=(const VulkanDescriptorManager&) = delete;

        void initialize(uint32_t maxSetsPerPool = 1000);
        void cleanup(const VulkanDevice& device);

        // Descriptor set layout management
        VkDescriptorSetLayout getOrCreateLayout(const VulkanDevice& device, 
                                               const std::vector<VkDescriptorSetLayoutBinding>& bindings);
        
        // Standard layouts for common use cases
        VkDescriptorSetLayout getTextureLayout(const VulkanDevice& device);
        VkDescriptorSetLayout getUniformBufferLayout(const VulkanDevice& device);
        VkDescriptorSetLayout getCombinedLayout(const VulkanDevice& device); // UBO + Texture

        // Descriptor set allocation
        VkDescriptorSet allocateDescriptorSet(const VulkanDevice& device, VkDescriptorSetLayout layout);
        // Return a set to its pool. Caller must ensure no pending command buffer still uses it.
        void freeDescriptorSet(const VulkanDevice& device, VkDescriptorSet descriptorSet);
        
        // Descriptor set updates
        void updateTextureDescriptorSet(const VulkanDevice& device, VkDescriptorSet descriptorSet,
                                       uint32_t binding, VkImageView imageView, VkSampler sampler);
        void updateUniformBufferDescriptorSet(const VulkanDevice& device, VkDescriptorSet descriptorSet,
                                             uint32_t binding, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range);

        // Resource management
        void resetPools(const VulkanDevice& device);
        size_t getLayoutCacheSize() const { return layoutCache.size(); }
        size_t getPoolCount() const { return descriptorPools.size(); }

    private:
        VkDescriptorSetLayout createLayout(const VulkanDevice& device, 
                                          const std::vector<VkDescriptorSetLayoutBinding>& bindings);
        VulkanDescriptorPool* getAvailablePool(const VulkanDevice& device);
        void createNewPool(const VulkanDevice& device);
    };
}

#endif