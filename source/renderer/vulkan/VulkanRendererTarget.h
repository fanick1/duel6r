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

#ifndef DUEL6_RENDERER_VULKAN_RENDERER_TARGET_H
#define DUEL6_RENDERER_VULKAN_RENDERER_TARGET_H

#include <vulkan/vulkan.h>
#include "../RendererTarget.h"

namespace Duel6 {

    class VulkanRenderer;

    class VulkanRendererTarget : public RendererTarget {
    private:
        VulkanRenderer& renderer;
        uint32_t width;
        uint32_t height;

        // Offscreen color image
        VkImage colorImage;
        VkDeviceMemory colorMemory;
        VkImageView colorView;

        // Offscreen depth+stencil image
        VkImage depthImage;
        VkDeviceMemory depthMemory;
        VkImageView depthView;

        // Offscreen render pass and framebuffer
        VkRenderPass offscreenRenderPass;
        VkFramebuffer offscreenFramebuffer;

        // Sampler and descriptor set for sampling the color image in apply()
        VkSampler sampler;
        VkDescriptorSet colorDescriptorSet;

        // Depth sampling view and descriptor set for depth blit in apply()
        VkImageView depthSamplingView;
        VkDescriptorSet depthDescriptorSet;

        // Texture ID registered with resource manager for apply() quad rendering

        bool resourcesCreated;

    public:
        VulkanRendererTarget(VulkanRenderer& renderer, ScreenParameters screenParameters);
        ~VulkanRendererTarget();

        void record(RenderCallback renderCallback) override;
        void apply(const Color& modulateColor) override;
        void resize(Int32 width, Int32 height) override;
        void rerender() override;

    private:
        void createResources();
        void destroyResources();
        void createOffscreenRenderPass();
    };

} // namespace Duel6

#endif // DUEL6_RENDERER_VULKAN_RENDERER_TARGET_H
