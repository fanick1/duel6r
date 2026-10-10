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

#ifndef DUEL6_RENDERER_VULKANTEXTURE_H
#define DUEL6_RENDERER_VULKANTEXTURE_H

#include <vulkan/vulkan.h>
#include "VulkanMemory.h"
#include "VulkanDevice.h"
#include "VulkanTypes.h"
#include "../../Image.h"
#include "../../console/Console.h"

namespace Duel6 {
    class VulkanSampler;

    class VulkanTexture {
    private:
        VulkanImage image;
        uint32_t width;
        uint32_t height;
        uint32_t depth;  // Array layers (1 for regular 2D, >1 for 2D array)
        VkFormat format;
        bool isValid;
        Console* console;

    public:
        explicit VulkanTexture(Console& console);
        ~VulkanTexture();

        // Non-copyable
        VulkanTexture(const VulkanTexture&) = delete;
        VulkanTexture& operator=(const VulkanTexture&) = delete;

        // Create texture from Image
        void create(const VulkanDevice& device, VulkanMemoryManager& memoryManager, const Image& image);
        

        void destroy(const VulkanDevice& device, VulkanMemoryManager& memoryManager);

        // Update texture data

        // Getters
        const VulkanImage& getImage() const { return image; }
        uint32_t getWidth() const { return width; }
        uint32_t getHeight() const { return height; }
        uint32_t getDepth() const { return depth; }

        // Set descriptor set (called by descriptor manager)

    private:
        void uploadImageData(const VulkanDevice& device, VulkanMemoryManager& memoryManager, const Image& image);
    };

}

#endif