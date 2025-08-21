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

#ifndef DUEL6_RENDERER_VULKANTYPES_H
#define DUEL6_RENDERER_VULKANTYPES_H

#include <vulkan/vulkan.h>
#include <vector>
#include "../RendererTypes.h"
#include "../../math/Vector.h"
#include "../../math/Matrix.h"

namespace Duel6 {
    // Forward declarations
    class VulkanTexture;
    class VulkanResourceManager;

    // Resource handle types
    using TextureId = uint32_t;
    using BufferId = uint32_t;
    constexpr TextureId INVALID_TEXTURE_ID = 0;
    constexpr BufferId INVALID_BUFFER_ID = 0;
    
    // Texture handle for Vulkan renderer (using resource ID instead of VkDescriptorSet)
    using Texture = TextureId;

    // Texture filtering modes
    enum class TextureFilter {
        Nearest,
        Linear
    };

    // Blending modes
    enum class BlendFunc {
        None,
        SrcAlpha,
        SrcColor
    };

    // Vertex attribute descriptions
    struct VertexAttribute {
        uint32_t location;
        VkFormat format;
        uint32_t offset;
    };

    struct VertexInputDescription {
        std::vector<VkVertexInputBindingDescription> bindings;
        std::vector<VkVertexInputAttributeDescription> attributes;
    };

    // Pipeline state
    struct PipelineState {
        VkPrimitiveTopology topology;
        bool depthTestEnable;
        bool depthWriteEnable;
        VkCompareOp depthCompareOp;
        bool blendEnable;
        VkBlendFactor srcColorBlendFactor;
        VkBlendFactor dstColorBlendFactor;
        VkBlendOp colorBlendOp;
        bool wireframe;
        VkCullModeFlags cullMode;
        
        PipelineState() 
            : topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
            , depthTestEnable(false)
            , depthWriteEnable(false)
            , depthCompareOp(VK_COMPARE_OP_LESS)
            , blendEnable(false)
            , srcColorBlendFactor(VK_BLEND_FACTOR_SRC_ALPHA)
            , dstColorBlendFactor(VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA)
            , colorBlendOp(VK_BLEND_OP_ADD)
            , wireframe(false)
            , cullMode(VK_CULL_MODE_NONE) {}
    };

    // Conversion functions
    VkFilter toVulkanFilter(TextureFilter filter);
    VkSamplerAddressMode toVulkanAddressMode(bool clamp);
    VkBlendFactor toVulkanBlendFactor(BlendFunc blendFunc);
    VkBlendOp toVulkanBlendOp(BlendFunc blendFunc);
    VkPrimitiveTopology toVulkanTopology(int primitiveType);
    VkFormat toVulkanFormat(int componentCount, bool normalized = false);
    
    // Utility functions
    uint32_t getFormatSize(VkFormat format);
    bool isDepthFormat(VkFormat format);
    bool isStencilFormat(VkFormat format);
    
    // Standard vertex formats
    namespace VertexFormats {
        // Position only (3D) - for color shaders
        struct ColorVertex {
            Vector position;
            
            static VertexInputDescription getDescription();

            static VkVertexInputBindingDescription getBindingDescription() {
                VkVertexInputBindingDescription bindingDescription{};
                bindingDescription.binding = 0;
                bindingDescription.stride = sizeof(ColorVertex);
                bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
                return bindingDescription;
            }

            static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions() {
                std::vector<VkVertexInputAttributeDescription> attributeDescriptions(1);
                
                attributeDescriptions[0].binding = 0;
                attributeDescriptions[0].location = 0;
                attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
                attributeDescriptions[0].offset = offsetof(ColorVertex, position);

                return attributeDescriptions;
            }
        };

        // Position + texture coords + texture index + flags - for material shaders  
        struct MaterialVertex {
            Vector position;     // 3D position
            Vector textureCoord; // UV coordinates (using z for texture index)
            uint32_t flags;      // Vertex flags (water wave, etc.)
            
            static VertexInputDescription getDescription();

            static VkVertexInputBindingDescription getBindingDescription() {
                VkVertexInputBindingDescription bindingDescription{};
                bindingDescription.binding = 0;
                bindingDescription.stride = sizeof(MaterialVertex);
                bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
                return bindingDescription;
            }

            static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions() {
                std::vector<VkVertexInputAttributeDescription> attributeDescriptions(4);
                
                // Position
                attributeDescriptions[0].binding = 0;
                attributeDescriptions[0].location = 0;
                attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
                attributeDescriptions[0].offset = offsetof(MaterialVertex, position);

                // Texture coordinates (u, v)
                attributeDescriptions[1].binding = 0;
                attributeDescriptions[1].location = 1;
                attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
                attributeDescriptions[1].offset = offsetof(MaterialVertex, textureCoord);

                // Texture index (z component)
                attributeDescriptions[2].binding = 0;
                attributeDescriptions[2].location = 2;
                attributeDescriptions[2].format = VK_FORMAT_R32_SFLOAT;
                attributeDescriptions[2].offset = offsetof(MaterialVertex, textureCoord) + 2 * sizeof(float);

                // Flags
                attributeDescriptions[3].binding = 0;
                attributeDescriptions[3].location = 3;
                attributeDescriptions[3].format = VK_FORMAT_R32_UINT;
                attributeDescriptions[3].offset = offsetof(MaterialVertex, flags);

                return attributeDescriptions;
            }
        };
        
        // Position + Color
        struct PositionColorVertex {
            float x, y, z;
            float r, g, b, a;
            
            static VertexInputDescription getDescription();
        };
        
        // Position + Texture coordinates
        struct PositionTexVertex {
            float x, y, z;
            float u, v;
            
            static VertexInputDescription getDescription();
        };
        
        // Position + Color + Texture coordinates
        struct PositionColorTexVertex {
            float x, y, z;
            float r, g, b, a;
            float u, v;
            
            static VertexInputDescription getDescription();
        };
    }

    // Uniform Buffer Objects for Vulkan shaders
    namespace UBO {
        // UBO for color shaders (vertex shader)
        struct ColorVertexUBO {
            alignas(16) Matrix mvp;
        };

        // UBO for color fragment shader
        struct ColorFragmentUBO {
            alignas(16) Vector color;
            alignas(4) float padding; // Align to vec4
        };

        // UBO for material shaders (vertex shader)
        struct MaterialVertexUBO {
            alignas(16) Matrix mvp;
            alignas(4) float globalTime;
            float padding[3]; // Align to 16 bytes
        };

        // UBO for material fragment shader
        struct MaterialFragmentUBO {
            alignas(4) uint32_t alphaTest;
            alignas(16) Vector modulateColor;
            alignas(4) float globalTime;
            float padding[3]; // Align to 16 bytes
        };

        // UBO for bloom filter fragment shader
        struct BloomFilterFragmentUBO {
            alignas(4) uint32_t alphaTest;
            alignas(16) Vector modulateColor;
            alignas(4) float globalTime;
            float padding[3]; // Align to 16 bytes
        };
    }
}

#endif