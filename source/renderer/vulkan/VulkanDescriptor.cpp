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

#include "VulkanDescriptor.h"
#include "VulkanContext.h"
#include "../../Format.h"

namespace Duel6 {
    // VulkanDescriptorPool implementation
    VulkanDescriptorPool::VulkanDescriptorPool(Console& console)
        : pool(VK_NULL_HANDLE), maxSets(0), allocatedSets(0), console(&console) {
    }

    VulkanDescriptorPool::~VulkanDescriptorPool() {
        if (pool != VK_NULL_HANDLE) {
            console->printLine("Warning: VulkanDescriptorPool destroyed without cleanup");
        }
    }

    void VulkanDescriptorPool::create(const VulkanDevice& device, uint32_t sets, 
                                     const std::vector<VkDescriptorPoolSize>& sizes) {
        maxSets = sets;
        poolSizes = sizes;
        allocatedSets = 0;

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = maxSets;

        VkResult result = vkCreateDescriptorPool(device.getLogicalDevice(), &poolInfo, nullptr, &pool);
        if (result != VK_SUCCESS) {
            D6_THROW(VulkanException, Format("Failed to create descriptor pool, error: {0}") << result);
        }
    }

    void VulkanDescriptorPool::destroy(const VulkanDevice& device) {
        if (pool != VK_NULL_HANDLE) {
            vkDestroyDescriptorPool(device.getLogicalDevice(), pool, nullptr);
            pool = VK_NULL_HANDLE;
        }
        allocatedSets = 0;
    }

    void VulkanDescriptorPool::reset(const VulkanDevice& device) {
        if (pool != VK_NULL_HANDLE) {
            vkResetDescriptorPool(device.getLogicalDevice(), pool, 0);
            allocatedSets = 0;
        }
    }

    VkDescriptorSet VulkanDescriptorPool::allocateDescriptorSet(const VulkanDevice& device, VkDescriptorSetLayout layout) {
        if (!canAllocate()) {
            return VK_NULL_HANDLE;
        }

        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = pool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &layout;

        VkDescriptorSet descriptorSet;
        VkResult result = vkAllocateDescriptorSets(device.getLogicalDevice(), &allocInfo, &descriptorSet);
        if (result != VK_SUCCESS) {
            return VK_NULL_HANDLE;
        }

        allocatedSets++;
        return descriptorSet;
    }

    void VulkanDescriptorPool::freeDescriptorSet(const VulkanDevice& device, VkDescriptorSet descriptorSet) {
        if (pool == VK_NULL_HANDLE || descriptorSet == VK_NULL_HANDLE) {
            return;
        }
        vkFreeDescriptorSets(device.getLogicalDevice(), pool, 1, &descriptorSet);
        if (allocatedSets > 0) {
            allocatedSets--;
        }
    }

    // VulkanDescriptorManager implementation
    VulkanDescriptorManager::VulkanDescriptorManager(Console& console)
        : currentPoolIndex(0), maxSetsPerPool(1000), console(&console) {
    }

    VulkanDescriptorManager::~VulkanDescriptorManager() {
        if (!layoutCache.empty() || !descriptorPools.empty()) {
            console->printLine("Warning: VulkanDescriptorManager destroyed without cleanup");
        }
    }

    void VulkanDescriptorManager::initialize(uint32_t maxSets) {
        maxSetsPerPool = maxSets;
        console->printLine("...Vulkan descriptor manager initialized");
    }

    void VulkanDescriptorManager::cleanup(const VulkanDevice& device) {
        // Destroy descriptor pools
        for (auto& pool : descriptorPools) {
            pool->destroy(device);
        }
        descriptorPools.clear();
        descriptorSetOwners.clear();

        // Destroy descriptor set layouts
        for (auto& pair : layoutCache) {
            vkDestroyDescriptorSetLayout(device.getLogicalDevice(), pair.second, nullptr);
        }
        layoutCache.clear();

        currentPoolIndex = 0;
        console->printLine("...Vulkan descriptor manager cleaned up");
    }

    VkDescriptorSetLayout VulkanDescriptorManager::getOrCreateLayout(const VulkanDevice& device,
                                                                    const std::vector<VkDescriptorSetLayoutBinding>& bindings) {
        DescriptorSetLayoutKey key = {bindings};
        
        auto it = layoutCache.find(key);
        if (it != layoutCache.end()) {
            return it->second;
        }

        VkDescriptorSetLayout layout = createLayout(device, bindings);
        layoutCache[key] = layout;
        return layout;
    }

    VkDescriptorSetLayout VulkanDescriptorManager::getTextureLayout(const VulkanDevice& device) {
        std::vector<VkDescriptorSetLayoutBinding> bindings(1);
        bindings[0].binding = 0;
        bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        bindings[0].descriptorCount = 1;
        bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        bindings[0].pImmutableSamplers = nullptr;

        return getOrCreateLayout(device, bindings);
    }

    VkDescriptorSetLayout VulkanDescriptorManager::getUniformBufferLayout(const VulkanDevice& device) {
        std::vector<VkDescriptorSetLayoutBinding> bindings(1);
        bindings[0].binding = 0;
        bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        bindings[0].descriptorCount = 1;
        bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        bindings[0].pImmutableSamplers = nullptr;

        return getOrCreateLayout(device, bindings);
    }

    VkDescriptorSetLayout VulkanDescriptorManager::getCombinedLayout(const VulkanDevice& device) {
        std::vector<VkDescriptorSetLayoutBinding> bindings(2);
        
        // Uniform buffer binding
        bindings[0].binding = 0;
        bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        bindings[0].descriptorCount = 1;
        bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        bindings[0].pImmutableSamplers = nullptr;

        // Combined image sampler binding
        bindings[1].binding = 1;
        bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        bindings[1].descriptorCount = 1;
        bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        bindings[1].pImmutableSamplers = nullptr;

        return getOrCreateLayout(device, bindings);
    }

    VkDescriptorSet VulkanDescriptorManager::allocateDescriptorSet(const VulkanDevice& device, VkDescriptorSetLayout layout) {
        VulkanDescriptorPool* pool = getAvailablePool(device);
        VkDescriptorSet descriptorSet = pool ? pool->allocateDescriptorSet(device, layout) : VK_NULL_HANDLE;
        if (descriptorSet == VK_NULL_HANDLE) {
            // All pools full (or fragmented) - grow with a fresh pool
            createNewPool(device);
            pool = descriptorPools.back().get();
            descriptorSet = pool->allocateDescriptorSet(device, layout);
        }

        if (descriptorSet == VK_NULL_HANDLE) {
            D6_THROW(VulkanException, "Failed to allocate descriptor set");
        }

        descriptorSetOwners[descriptorSet] = pool;
        return descriptorSet;
    }

    void VulkanDescriptorManager::freeDescriptorSet(const VulkanDevice& device, VkDescriptorSet descriptorSet) {
        auto it = descriptorSetOwners.find(descriptorSet);
        if (it == descriptorSetOwners.end()) {
            return;
        }
        it->second->freeDescriptorSet(device, descriptorSet);
        descriptorSetOwners.erase(it);
    }

    void VulkanDescriptorManager::updateTextureDescriptorSet(const VulkanDevice& device, VkDescriptorSet descriptorSet,
                                                            uint32_t binding, VkImageView imageView, VkSampler sampler) {
        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = imageView;
        imageInfo.sampler = sampler;

        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = descriptorSet;
        descriptorWrite.dstBinding = binding;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(device.getLogicalDevice(), 1, &descriptorWrite, 0, nullptr);
    }

    void VulkanDescriptorManager::updateUniformBufferDescriptorSet(const VulkanDevice& device, VkDescriptorSet descriptorSet,
                                                                  uint32_t binding, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range) {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = buffer;
        bufferInfo.offset = offset;
        bufferInfo.range = range;

        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = descriptorSet;
        descriptorWrite.dstBinding = binding;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pBufferInfo = &bufferInfo;

        vkUpdateDescriptorSets(device.getLogicalDevice(), 1, &descriptorWrite, 0, nullptr);
    }

    void VulkanDescriptorManager::resetPools(const VulkanDevice& device) {
        for (auto& pool : descriptorPools) {
            pool->reset(device);
        }
        descriptorSetOwners.clear();
        currentPoolIndex = 0;
    }

    VkDescriptorSetLayout VulkanDescriptorManager::createLayout(const VulkanDevice& device,
                                                               const std::vector<VkDescriptorSetLayoutBinding>& bindings) {
        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings = bindings.data();

        VkDescriptorSetLayout layout;
        VkResult result = vkCreateDescriptorSetLayout(device.getLogicalDevice(), &layoutInfo, nullptr, &layout);
        if (result != VK_SUCCESS) {
            D6_THROW(VulkanException, Format("Failed to create descriptor set layout, error: {0}") << result);
        }

        return layout;
    }

    VulkanDescriptorPool* VulkanDescriptorManager::getAvailablePool(const VulkanDevice& device) {
        if (descriptorPools.empty()) {
            createNewPool(device);
        }

        // Find a pool that has space (scan from start: freed sets reopen earlier pools)
        for (size_t i = 0; i < descriptorPools.size(); ++i) {
            if (descriptorPools[i]->canAllocate()) {
                currentPoolIndex = i;
                return descriptorPools[i].get();
            }
        }

        // No available pool found
        return nullptr;
    }

    void VulkanDescriptorManager::createNewPool(const VulkanDevice& device) {
        auto pool = std::make_unique<VulkanDescriptorPool>(*console);

        std::vector<VkDescriptorPoolSize> poolSizes = {
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, maxSetsPerPool},
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, maxSetsPerPool}
        };

        pool->create(device, maxSetsPerPool, poolSizes);
        descriptorPools.push_back(std::move(pool));
    }
}