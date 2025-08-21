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

    VkBlendFactor toVulkanBlendFactor(BlendFunc blendFunc) {
        switch (blendFunc) {
            case BlendFunc::None:
                return VK_BLEND_FACTOR_ONE;
            case BlendFunc::SrcAlpha:
                return VK_BLEND_FACTOR_SRC_ALPHA;
            case BlendFunc::SrcColor:
                return VK_BLEND_FACTOR_SRC_COLOR;
            default:
                return VK_BLEND_FACTOR_ONE;
        }
    }

    VkBlendOp toVulkanBlendOp(BlendFunc blendFunc) {
        // For now, always use additive blending
        return VK_BLEND_OP_ADD;
    }

    VkPrimitiveTopology toVulkanTopology(int primitiveType) {
        // Map from OpenGL-style primitive types
        switch (primitiveType) {
            case 0: return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
            case 1: return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
            case 2: return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
            case 3: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            case 4: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
            case 5: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
            default: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        }
    }

    VkFormat toVulkanFormat(int componentCount, bool normalized) {
        if (normalized) {
            switch (componentCount) {
                case 1: return VK_FORMAT_R8_UNORM;
                case 2: return VK_FORMAT_R8G8_UNORM;
                case 3: return VK_FORMAT_R8G8B8_UNORM;
                case 4: return VK_FORMAT_R8G8B8A8_UNORM;
                default: return VK_FORMAT_R32G32B32A32_SFLOAT;
            }
        } else {
            switch (componentCount) {
                case 1: return VK_FORMAT_R32_SFLOAT;
                case 2: return VK_FORMAT_R32G32_SFLOAT;
                case 3: return VK_FORMAT_R32G32B32_SFLOAT;
                case 4: return VK_FORMAT_R32G32B32A32_SFLOAT;
                default: return VK_FORMAT_R32G32B32A32_SFLOAT;
            }
        }
    }

    uint32_t getFormatSize(VkFormat format) {
        switch (format) {
            case VK_FORMAT_R8_UNORM:
            case VK_FORMAT_R8_UINT:
            case VK_FORMAT_R8_SINT:
                return 1;
            case VK_FORMAT_R8G8_UNORM:
            case VK_FORMAT_R8G8_UINT:
            case VK_FORMAT_R8G8_SINT:
            case VK_FORMAT_R16_SFLOAT:
            case VK_FORMAT_R16_UINT:
            case VK_FORMAT_R16_SINT:
                return 2;
            case VK_FORMAT_R8G8B8_UNORM:
            case VK_FORMAT_R8G8B8_UINT:
            case VK_FORMAT_R8G8B8_SINT:
                return 3;
            case VK_FORMAT_R8G8B8A8_UNORM:
            case VK_FORMAT_R8G8B8A8_UINT:
            case VK_FORMAT_R8G8B8A8_SINT:
            case VK_FORMAT_R16G16_SFLOAT:
            case VK_FORMAT_R16G16_UINT:
            case VK_FORMAT_R16G16_SINT:
            case VK_FORMAT_R32_SFLOAT:
            case VK_FORMAT_R32_UINT:
            case VK_FORMAT_R32_SINT:
                return 4;
            case VK_FORMAT_R16G16B16_SFLOAT:
            case VK_FORMAT_R16G16B16_UINT:
            case VK_FORMAT_R16G16B16_SINT:
                return 6;
            case VK_FORMAT_R16G16B16A16_SFLOAT:
            case VK_FORMAT_R16G16B16A16_UINT:
            case VK_FORMAT_R16G16B16A16_SINT:
            case VK_FORMAT_R32G32_SFLOAT:
            case VK_FORMAT_R32G32_UINT:
            case VK_FORMAT_R32G32_SINT:
                return 8;
            case VK_FORMAT_R32G32B32_SFLOAT:
            case VK_FORMAT_R32G32B32_UINT:
            case VK_FORMAT_R32G32B32_SINT:
                return 12;
            case VK_FORMAT_R32G32B32A32_SFLOAT:
            case VK_FORMAT_R32G32B32A32_UINT:
            case VK_FORMAT_R32G32B32A32_SINT:
                return 16;
            default:
                return 4; // Default fallback
        }
    }

    bool isDepthFormat(VkFormat format) {
        switch (format) {
            case VK_FORMAT_D16_UNORM:
            case VK_FORMAT_D32_SFLOAT:
            case VK_FORMAT_D16_UNORM_S8_UINT:
            case VK_FORMAT_D24_UNORM_S8_UINT:
            case VK_FORMAT_D32_SFLOAT_S8_UINT:
                return true;
            default:
                return false;
        }
    }

    bool isStencilFormat(VkFormat format) {
        switch (format) {
            case VK_FORMAT_S8_UINT:
            case VK_FORMAT_D16_UNORM_S8_UINT:
            case VK_FORMAT_D24_UNORM_S8_UINT:
            case VK_FORMAT_D32_SFLOAT_S8_UINT:
                return true;
            default:
                return false;
        }
    }

    // Standard vertex format descriptions
    namespace VertexFormats {
        VertexInputDescription ColorVertex::getDescription() {
            VertexInputDescription description;

            VkVertexInputBindingDescription binding{};
            binding.binding = 0;
            binding.stride = sizeof(ColorVertex);
            binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
            description.bindings.push_back(binding);

            VkVertexInputAttributeDescription attribute{};
            attribute.binding = 0;
            attribute.location = 0;
            attribute.format = VK_FORMAT_R32G32B32_SFLOAT;
            attribute.offset = offsetof(ColorVertex, position);
            description.attributes.push_back(attribute);

            return description;
        }

        VertexInputDescription MaterialVertex::getDescription() {
            VertexInputDescription description;

            VkVertexInputBindingDescription binding{};
            binding.binding = 0;
            binding.stride = sizeof(MaterialVertex);
            binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
            description.bindings.push_back(binding);

            // Position
            VkVertexInputAttributeDescription posAttribute{};
            posAttribute.binding = 0;
            posAttribute.location = 0;
            posAttribute.format = VK_FORMAT_R32G32B32_SFLOAT;
            posAttribute.offset = offsetof(MaterialVertex, position);
            description.attributes.push_back(posAttribute);

            // Texture coordinates (u, v)
            VkVertexInputAttributeDescription texAttribute{};
            texAttribute.binding = 0;
            texAttribute.location = 1;
            texAttribute.format = VK_FORMAT_R32G32_SFLOAT;
            texAttribute.offset = offsetof(MaterialVertex, textureCoord);
            description.attributes.push_back(texAttribute);

            // Texture index (z component of textureCoord)
            VkVertexInputAttributeDescription texIndexAttribute{};
            texIndexAttribute.binding = 0;
            texIndexAttribute.location = 2;
            texIndexAttribute.format = VK_FORMAT_R32_SFLOAT;
            texIndexAttribute.offset = offsetof(MaterialVertex, textureCoord) + 2 * sizeof(float);
            description.attributes.push_back(texIndexAttribute);

            // Flags
            VkVertexInputAttributeDescription flagsAttribute{};
            flagsAttribute.binding = 0;
            flagsAttribute.location = 3;
            flagsAttribute.format = VK_FORMAT_R32_UINT;
            flagsAttribute.offset = offsetof(MaterialVertex, flags);
            description.attributes.push_back(flagsAttribute);

            return description;
        }

        VertexInputDescription PositionColorVertex::getDescription() {
            VertexInputDescription description;

            VkVertexInputBindingDescription binding{};
            binding.binding = 0;
            binding.stride = sizeof(PositionColorVertex);
            binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
            description.bindings.push_back(binding);

            // Position attribute
            VkVertexInputAttributeDescription posAttr{};
            posAttr.binding = 0;
            posAttr.location = 0;
            posAttr.format = VK_FORMAT_R32G32B32_SFLOAT;
            posAttr.offset = 0;
            description.attributes.push_back(posAttr);

            // Color attribute
            VkVertexInputAttributeDescription colorAttr{};
            colorAttr.binding = 0;
            colorAttr.location = 1;
            colorAttr.format = VK_FORMAT_R32G32B32A32_SFLOAT;
            colorAttr.offset = sizeof(float) * 3;
            description.attributes.push_back(colorAttr);

            return description;
        }

        VertexInputDescription PositionTexVertex::getDescription() {
            VertexInputDescription description;

            VkVertexInputBindingDescription binding{};
            binding.binding = 0;
            binding.stride = sizeof(PositionTexVertex);
            binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
            description.bindings.push_back(binding);

            // Position attribute
            VkVertexInputAttributeDescription posAttr{};
            posAttr.binding = 0;
            posAttr.location = 0;
            posAttr.format = VK_FORMAT_R32G32B32_SFLOAT;
            posAttr.offset = 0;
            description.attributes.push_back(posAttr);

            // Texture coordinate attribute
            VkVertexInputAttributeDescription texAttr{};
            texAttr.binding = 0;
            texAttr.location = 1;
            texAttr.format = VK_FORMAT_R32G32_SFLOAT;
            texAttr.offset = sizeof(float) * 3;
            description.attributes.push_back(texAttr);

            return description;
        }

        VertexInputDescription PositionColorTexVertex::getDescription() {
            VertexInputDescription description;

            VkVertexInputBindingDescription binding{};
            binding.binding = 0;
            binding.stride = sizeof(PositionColorTexVertex);
            binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
            description.bindings.push_back(binding);

            // Position attribute
            VkVertexInputAttributeDescription posAttr{};
            posAttr.binding = 0;
            posAttr.location = 0;
            posAttr.format = VK_FORMAT_R32G32B32_SFLOAT;
            posAttr.offset = 0;
            description.attributes.push_back(posAttr);

            // Color attribute
            VkVertexInputAttributeDescription colorAttr{};
            colorAttr.binding = 0;
            colorAttr.location = 1;
            colorAttr.format = VK_FORMAT_R32G32B32A32_SFLOAT;
            colorAttr.offset = sizeof(float) * 3;
            description.attributes.push_back(colorAttr);

            // Texture coordinate attribute
            VkVertexInputAttributeDescription texAttr{};
            texAttr.binding = 0;
            texAttr.location = 2;
            texAttr.format = VK_FORMAT_R32G32_SFLOAT;
            texAttr.offset = sizeof(float) * 7;
            description.attributes.push_back(texAttr);

            return description;
        }
    }
}