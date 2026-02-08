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
#include <memory>
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
        VkSampler sampler;
        VkDescriptorSet descriptorSet;
        uint32_t width;
        uint32_t height;
        uint32_t depth;  // Array layers (1 for regular 2D, >1 for 2D array)
        VkFormat format;
        bool isValid;
        Console* console;

    public:
        explicit VulkanTexture(Console& console);
        ~VulkanTexture();

        // Non-copyable but moveable
        VulkanTexture(const VulkanTexture&) = delete;
        VulkanTexture& operator=(const VulkanTexture&) = delete;
        VulkanTexture(VulkanTexture&& other) noexcept;
        VulkanTexture& operator=(VulkanTexture&& other) noexcept;

        // Create texture from Image
        void create(const VulkanDevice& device, VulkanMemoryManager& memoryManager, 
                   const Image& image, TextureFilter filter, bool clamp);
        
        // Create empty texture with specific dimensions
        void create(const VulkanDevice& device, VulkanMemoryManager& memoryManager,
                   uint32_t width, uint32_t height, VkFormat format, TextureFilter filter, bool clamp);

        void destroy(const VulkanDevice& device, VulkanMemoryManager& memoryManager);

        // Update texture data
        void updateData(const VulkanDevice& device, VulkanMemoryManager& memoryManager, const Image& image);
        void updateSubData(const VulkanDevice& device, VulkanMemoryManager& memoryManager,
                          uint32_t x, uint32_t y, uint32_t width, uint32_t height, const void* data);

        // Getters
        const VulkanImage& getImage() const { return image; }
        VkSampler getSampler() const { return sampler; }
        VkDescriptorSet getDescriptorSet() const { return descriptorSet; }
        uint32_t getWidth() const { return width; }
        uint32_t getHeight() const { return height; }
        uint32_t getDepth() const { return depth; }
        VkFormat getFormat() const { return format; }
        bool valid() const { return isValid; }

        // Set descriptor set (called by descriptor manager)
        void setDescriptorSet(VkDescriptorSet ds) { descriptorSet = ds; }

    private:
        void createSampler(const VulkanDevice& device, TextureFilter filter, bool clamp);
        VkFormat selectFormat(const Image& image) const;
        void uploadImageData(const VulkanDevice& device, VulkanMemoryManager& memoryManager, const Image& image);
    };

    // Texture handle type for the renderer interface
    using TextureHandle = std::shared_ptr<VulkanTexture>;
}

#endif