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

#include "VulkanTypes.h"
#include <cstddef>

namespace Duel6 {
    VkFilter toVulkanFilter(TextureFilter filter) {
        switch (filter) {
            case TextureFilter::Nearest:
                return VK_FILTER_NEAREST;
            case TextureFilter::Linear:
                return VK_FILTER_LINEAR;
            default:
                return VK_FILTER_LINEAR;
        }
    }

    VkSamplerAddressMode toVulkanAddressMode(bool clamp) {
        return clamp ? VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE : VK_SAMPLER_ADDRESS_MODE_REPEAT;
    }

    // Standard vertex format descriptions
    VkVertexInputBindingDescription VulkanVertex::getBindingDescription() {
        VkVertexInputBindingDescription binding{};
        binding.binding = 0;
        binding.stride = sizeof(VulkanVertex);
        binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return binding;
    }

    std::vector<VkVertexInputAttributeDescription> VulkanVertex::getAttributeDescriptions() {
        return {
            {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VulkanVertex, position)},
            {1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VulkanVertex, textureCoord)},
            {2, 0, VK_FORMAT_R8G8B8A8_UNORM, offsetof(VulkanVertex, color)},
            {3, 0, VK_FORMAT_R32_UINT, offsetof(VulkanVertex, flags)},
        };
    }
}