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

#include "VulkanSampler.h"
#include "VulkanContext.h"
#include "../../Format.h"

namespace Duel6 {
    VulkanSamplerManager::VulkanSamplerManager(Console& console)
        : console(&console) {
    }

    VulkanSamplerManager::~VulkanSamplerManager() {
        if (!samplerCache.empty()) {
            console->printLine("Warning: VulkanSamplerManager destroyed with samplers still cached");
        }
    }


    void VulkanSamplerManager::cleanup(const VulkanDevice& device) {
        for (auto& pair : samplerCache) {
            vkDestroySampler(device.getLogicalDevice(), pair.second, nullptr);
        }
        samplerCache.clear();
        console->printLine(Format("...Vulkan sampler manager cleaned up"));
    }

        VkSampler VulkanSamplerManager::getSampler(const VulkanDevice& device, TextureFilter filter, bool clamp) {
        SamplerKey key = {filter, clamp};
        
        auto it = samplerCache.find(key);
        if (it != samplerCache.end()) {
            return it->second;
        }

        // Create new sampler
        VkSampler sampler = createSampler(device, key);
        samplerCache[key] = sampler;
        
        return sampler;
    }

    VkSampler VulkanSamplerManager::getNearestSampler(const VulkanDevice& device, bool clamp) {
        return getSampler(device, TextureFilter::Nearest, clamp);
    }

    VkSampler VulkanSamplerManager::getLinearSampler(const VulkanDevice& device, bool clamp) {
        return getSampler(device, TextureFilter::Linear, clamp);
    }

    VkSampler VulkanSamplerManager::createSampler(const VulkanDevice& device, const SamplerKey& key) {
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = toVulkanFilter(key.filter);
        samplerInfo.minFilter = toVulkanFilter(key.filter);
        samplerInfo.addressModeU = toVulkanAddressMode(key.clamp);
        samplerInfo.addressModeV = toVulkanAddressMode(key.clamp);
        samplerInfo.addressModeW = toVulkanAddressMode(key.clamp);
        
                samplerInfo.anisotropyEnable = VK_FALSE;
        samplerInfo.maxAnisotropy = 1.0f;

        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

        // Mipmap settings
        samplerInfo.mipmapMode = (key.filter == TextureFilter::Linear) ? 
            VK_SAMPLER_MIPMAP_MODE_LINEAR : VK_SAMPLER_MIPMAP_MODE_NEAREST;
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = 0.0f; // No mipmaps for now

        VkSampler sampler;
        VkResult result = vkCreateSampler(device.getLogicalDevice(), &samplerInfo, nullptr, &sampler);
        if (result != VK_SUCCESS) {
            D6_THROW(VulkanException, Format("Failed to create sampler, error: {0}") << result);
        }

        return sampler;
    }
}