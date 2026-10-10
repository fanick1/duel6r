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
    constexpr TextureId INVALID_TEXTURE_ID = 0;
    
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

    VkFilter toVulkanFilter(TextureFilter filter);
    VkSamplerAddressMode toVulkanAddressMode(bool clamp);

    // The renderer's only vertex format
    struct VulkanVertex {
        Vector position;
        Vector textureCoord;  // u, v, texture layer
        uint32_t color;       // RGBA, one byte each
        uint32_t flags;       // Duel6::Vertex::Flag (water wave)

        static VkVertexInputBindingDescription getBindingDescription();
        static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
    };

    // Layout of the push constant block shared by the vertex and fragment shaders
    struct PushConstants {
        Float32 mvp[16];
        Float32 modulateColor[4];
        uint32_t alphaTest;
        Float32 time;
    };
    constexpr uint32_t PUSH_CONSTANTS_SIZE = sizeof(PushConstants);
}

#endif