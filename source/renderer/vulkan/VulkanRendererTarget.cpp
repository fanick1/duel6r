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

#include "VulkanRendererTarget.h"
#include "VulkanRenderer.h"
#include "../../VideoException.h"

namespace Duel6 {

    VulkanRendererTarget::VulkanRendererTarget(VulkanRenderer& renderer, ScreenParameters screenParameters)
        : renderer(renderer)
        , width(screenParameters.getClientWidth())
        , height(screenParameters.getClientHeight())
        , colorImage(VK_NULL_HANDLE)
        , colorMemory(VK_NULL_HANDLE)
        , colorView(VK_NULL_HANDLE)
        , depthImage(VK_NULL_HANDLE)
        , depthMemory(VK_NULL_HANDLE)
        , depthView(VK_NULL_HANDLE)
        , offscreenRenderPass(VK_NULL_HANDLE)
        , offscreenFramebuffer(VK_NULL_HANDLE)
        , sampler(VK_NULL_HANDLE)
        , colorDescriptorSet(VK_NULL_HANDLE)
        , depthSamplingView(VK_NULL_HANDLE)
        , depthDescriptorSet(VK_NULL_HANDLE)
        , resourcesCreated(false) {
        createResources();
    }

    VulkanRendererTarget::~VulkanRendererTarget() {
        if (renderer.isInitialized()) {
            renderer.getDevice().waitIdle();
        }
        destroyResources();
    }

    void VulkanRendererTarget::createOffscreenRenderPass() {
        const auto& mainConfig = renderer.getMainRenderPass().getConfig();
        VkDevice dev = renderer.getDevice().getLogicalDevice();

        // Color attachment - same format as main, but finalLayout is SHADER_READ_ONLY
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = mainConfig.colorFormat;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        // Depth+stencil attachment - same format as main
        VkAttachmentDescription depthAttachment{};
        depthAttachment.format = mainConfig.depthFormat;
        depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        std::array<VkAttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};

        VkAttachmentReference colorRef{};
        colorRef.attachment = 0;
        colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentReference depthRef{};
        depthRef.attachment = 1;
        depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorRef;
        subpass.pDepthStencilAttachment = &depthRef;

        // Single dependency matching main render pass exactly for pipeline compatibility
        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        VkRenderPassCreateInfo rpInfo{};
        rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        rpInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        rpInfo.pAttachments = attachments.data();
        rpInfo.subpassCount = 1;
        rpInfo.pSubpasses = &subpass;
        rpInfo.dependencyCount = 1;
        rpInfo.pDependencies = &dependency;

        VkResult result = vkCreateRenderPass(dev, &rpInfo, nullptr, &offscreenRenderPass);
        if (result != VK_SUCCESS) {
            D6_THROW(VideoException, "Failed to create offscreen render pass");
        }
    }

    void VulkanRendererTarget::createResources() {
        if (width == 0 || height == 0 || !renderer.isInitialized()) {
            return;
        }

        VkDevice dev = renderer.getDevice().getLogicalDevice();
        const auto& mainConfig = renderer.getMainRenderPass().getConfig();

        // Create offscreen render pass (compatible with main)
        createOffscreenRenderPass();

        // Create color image
        {
            VkImageCreateInfo imageInfo{};
            imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            imageInfo.imageType = VK_IMAGE_TYPE_2D;
            imageInfo.format = mainConfig.colorFormat;
            imageInfo.extent = {width, height, 1};
            imageInfo.mipLevels = 1;
            imageInfo.arrayLayers = 1;
            imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
            imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
            imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
            imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

            if (vkCreateImage(dev, &imageInfo, nullptr, &colorImage) != VK_SUCCESS) {
                D6_THROW(VideoException, "Failed to create offscreen color image");
            }

            VkMemoryRequirements memReqs;
            vkGetImageMemoryRequirements(dev, colorImage, &memReqs);

            VkMemoryAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocInfo.allocationSize = memReqs.size;
            allocInfo.memoryTypeIndex = renderer.getDevice().findMemoryType(
                memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

            if (vkAllocateMemory(dev, &allocInfo, nullptr, &colorMemory) != VK_SUCCESS) {
                D6_THROW(VideoException, "Failed to allocate offscreen color memory");
            }
            vkBindImageMemory(dev, colorImage, colorMemory, 0);

            VkImageViewCreateInfo viewInfo{};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image = colorImage;
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;  // sampled as sampler2DArray like every texture
            viewInfo.format = mainConfig.colorFormat;
            viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            viewInfo.subresourceRange.baseMipLevel = 0;
            viewInfo.subresourceRange.levelCount = 1;
            viewInfo.subresourceRange.baseArrayLayer = 0;
            viewInfo.subresourceRange.layerCount = 1;

            if (vkCreateImageView(dev, &viewInfo, nullptr, &colorView) != VK_SUCCESS) {
                D6_THROW(VideoException, "Failed to create offscreen color image view");
            }
        }

        // Create depth+stencil image
        {
            VkImageCreateInfo imageInfo{};
            imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            imageInfo.imageType = VK_IMAGE_TYPE_2D;
            imageInfo.format = mainConfig.depthFormat;
            imageInfo.extent = {width, height, 1};
            imageInfo.mipLevels = 1;
            imageInfo.arrayLayers = 1;
            imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
            imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
            imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
            imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

            if (vkCreateImage(dev, &imageInfo, nullptr, &depthImage) != VK_SUCCESS) {
                D6_THROW(VideoException, "Failed to create offscreen depth image");
            }

            VkMemoryRequirements memReqs;
            vkGetImageMemoryRequirements(dev, depthImage, &memReqs);

            VkMemoryAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocInfo.allocationSize = memReqs.size;
            allocInfo.memoryTypeIndex = renderer.getDevice().findMemoryType(
                memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

            if (vkAllocateMemory(dev, &allocInfo, nullptr, &depthMemory) != VK_SUCCESS) {
                D6_THROW(VideoException, "Failed to allocate offscreen depth memory");
            }
            vkBindImageMemory(dev, depthImage, depthMemory, 0);

            VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            if (mainConfig.depthFormat == VK_FORMAT_D32_SFLOAT_S8_UINT ||
                mainConfig.depthFormat == VK_FORMAT_D24_UNORM_S8_UINT ||
                mainConfig.depthFormat == VK_FORMAT_D16_UNORM_S8_UINT) {
                aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
            }

            VkImageViewCreateInfo viewInfo{};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image = depthImage;
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;  // sampled as sampler2DArray like every texture
            viewInfo.format = mainConfig.depthFormat;
            viewInfo.subresourceRange.aspectMask = aspectMask;
            viewInfo.subresourceRange.baseMipLevel = 0;
            viewInfo.subresourceRange.levelCount = 1;
            viewInfo.subresourceRange.baseArrayLayer = 0;
            viewInfo.subresourceRange.layerCount = 1;

            if (vkCreateImageView(dev, &viewInfo, nullptr, &depthView) != VK_SUCCESS) {
                D6_THROW(VideoException, "Failed to create offscreen depth image view");
            }
        }

        // Create framebuffer
        {
            std::array<VkImageView, 2> attachments = {colorView, depthView};

            VkFramebufferCreateInfo fbInfo{};
            fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            fbInfo.renderPass = offscreenRenderPass;
            fbInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
            fbInfo.pAttachments = attachments.data();
            fbInfo.width = width;
            fbInfo.height = height;
            fbInfo.layers = 1;

            if (vkCreateFramebuffer(dev, &fbInfo, nullptr, &offscreenFramebuffer) != VK_SUCCESS) {
                D6_THROW(VideoException, "Failed to create offscreen framebuffer");
            }
        }

        // Create sampler for sampling the color attachment
        sampler = renderer.getSamplerManager().getLinearSampler(renderer.getDevice(), true);

        // Allocate descriptor set for the offscreen color image
        VkDescriptorSetLayout texLayout = renderer.getTextureDescriptorSetLayout();
        colorDescriptorSet = renderer.getDescriptorManager().allocateDescriptorSet(
            renderer.getDevice(), texLayout);

        // Update descriptor set to point to offscreen color image
        renderer.getDescriptorManager().updateTextureDescriptorSet(
            renderer.getDevice(), colorDescriptorSet, 0, colorView, sampler);

        // Create depth sampling view (depth-only aspect, required for shader sampling)
        {
            VkImageViewCreateInfo viewInfo{};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image = depthImage;
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;  // sampled as sampler2DArray like every texture
            viewInfo.format = mainConfig.depthFormat;
            viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            viewInfo.subresourceRange.baseMipLevel = 0;
            viewInfo.subresourceRange.levelCount = 1;
            viewInfo.subresourceRange.baseArrayLayer = 0;
            viewInfo.subresourceRange.layerCount = 1;

            if (vkCreateImageView(dev, &viewInfo, nullptr, &depthSamplingView) != VK_SUCCESS) {
                D6_THROW(VideoException, "Failed to create depth sampling image view");
            }
        }

        // Allocate descriptor set for depth sampling
        depthDescriptorSet = renderer.getDescriptorManager().allocateDescriptorSet(
            renderer.getDevice(), texLayout);
        renderer.getDescriptorManager().updateTextureDescriptorSet(
            renderer.getDevice(), depthDescriptorSet, 0, depthSamplingView,
            renderer.getSamplerManager().getNearestSampler(renderer.getDevice(), true));

        resourcesCreated = true;
    }

    void VulkanRendererTarget::destroyResources() {
        if (!resourcesCreated) {
            return;
        }

        VkDevice dev = renderer.getDevice().getLogicalDevice();

        if (offscreenFramebuffer != VK_NULL_HANDLE) {
            vkDestroyFramebuffer(dev, offscreenFramebuffer, nullptr);
            offscreenFramebuffer = VK_NULL_HANDLE;
        }

        if (depthSamplingView != VK_NULL_HANDLE) {
            vkDestroyImageView(dev, depthSamplingView, nullptr);
            depthSamplingView = VK_NULL_HANDLE;
        }
        if (depthView != VK_NULL_HANDLE) {
            vkDestroyImageView(dev, depthView, nullptr);
            depthView = VK_NULL_HANDLE;
        }
        if (depthImage != VK_NULL_HANDLE) {
            vkDestroyImage(dev, depthImage, nullptr);
            depthImage = VK_NULL_HANDLE;
        }
        if (depthMemory != VK_NULL_HANDLE) {
            vkFreeMemory(dev, depthMemory, nullptr);
            depthMemory = VK_NULL_HANDLE;
        }

        if (colorView != VK_NULL_HANDLE) {
            vkDestroyImageView(dev, colorView, nullptr);
            colorView = VK_NULL_HANDLE;
        }
        if (colorImage != VK_NULL_HANDLE) {
            vkDestroyImage(dev, colorImage, nullptr);
            colorImage = VK_NULL_HANDLE;
        }
        if (colorMemory != VK_NULL_HANDLE) {
            vkFreeMemory(dev, colorMemory, nullptr);
            colorMemory = VK_NULL_HANDLE;
        }

        if (offscreenRenderPass != VK_NULL_HANDLE) {
            vkDestroyRenderPass(dev, offscreenRenderPass, nullptr);
            offscreenRenderPass = VK_NULL_HANDLE;
        }

        // sampler is managed by SamplerManager, don't destroy it here
        // Descriptor sets must go back to the pool, otherwise every resize leaks two of them
        auto& descriptorManager = renderer.getDescriptorManager();
        descriptorManager.freeDescriptorSet(renderer.getDevice(), colorDescriptorSet);
        colorDescriptorSet = VK_NULL_HANDLE;
        descriptorManager.freeDescriptorSet(renderer.getDevice(), depthDescriptorSet);
        depthDescriptorSet = VK_NULL_HANDLE;

        resourcesCreated = false;
    }

    void VulkanRendererTarget::record(RenderCallback renderCallback) {
        callback = renderCallback;
        rerender();
    }

    void VulkanRendererTarget::rerender() {
        if (!callback || !resourcesCreated || !renderer.isInitialized()) {
            return;
        }

                // The game rerenders between frames (round start, resize, level change); supporting it
        // mid-frame would mean interrupting the main render pass for nothing
        if (renderer.isInFrame()) {
            D6_THROW(VideoException, "Offscreen targets can only be rendered between frames");
        }

        // The frame slot's vertex buffer may still be read by the GPU: wait before the batch renderer writes into it
        renderer.getDevice().waitIdle();
        VkCommandBuffer cmdBuf = renderer.getDevice().beginSingleTimeCommands();

        // Begin offscreen render pass
        VkRenderPassBeginInfo rpBegin{};
        rpBegin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        rpBegin.renderPass = offscreenRenderPass;
        rpBegin.framebuffer = offscreenFramebuffer;
        rpBegin.renderArea.offset = {0, 0};
        rpBegin.renderArea.extent = {width, height};

        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = {{0.0f, 0.0f, 0.0f, 0.0f}};
        clearValues[1].depthStencil = {1.0f, 0};

        rpBegin.clearValueCount = static_cast<uint32_t>(clearValues.size());
        rpBegin.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(cmdBuf, &rpBegin, VK_SUBPASS_CONTENTS_INLINE);

        // Set inFrame so renderer draw calls work
        renderer.setInFrame(true);

                renderer.getBatchRenderer().beginFrame(renderer.getCurrentFrameIndex());

        // The callback adds draw commands; the pass is rendered upside down like an OpenGL
        // framebuffer object, so shared-edge pixels are rasterised exactly as the OpenGL renderers do
        renderer.setOffscreen(true);
        callback();
        renderer.setOffscreen(false);

                renderer.getBatchRenderer().flush(cmdBuf, width, height, false);

        vkCmdEndRenderPass(cmdBuf);

        // Explicit barrier: ensure color attachment writes are visible to fragment shader reads.
        // The render pass finalLayout transitions the image to SHADER_READ_ONLY_OPTIMAL,
        // but we need an execution/memory dependency for correct synchronization.
        VkMemoryBarrier memBarrier{};
        memBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
        memBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        memBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        vkCmdPipelineBarrier(cmdBuf,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0, 1, &memBarrier, 0, nullptr, 0, nullptr);

                renderer.setInFrame(false);
        renderer.getDevice().endSingleTimeCommands(cmdBuf);
    }

    void VulkanRendererTarget::apply(const Color& modulateColor) {
        if (!resourcesCreated || !renderer.isInFrame()) {
            return;
        }

        // Fullscreen quad. The offscreen image is stored bottom row first, which is texture V = 0.
        Vector p1(0.0f, 0.0f, 0.0f);
        Vector p2(static_cast<float>(width), 0.0f, 0.0f);
        Vector p3(static_cast<float>(width), static_cast<float>(height), 0.0f);
        Vector p4(0.0f, static_cast<float>(height), 0.0f);
        // Clockwise like all game geometry (back faces are culled)
        uint32_t white = 0xFFFFFFFF;
        VulkanVertex vertices[6] = {
            {p1, Vector(0, 0, 0), white, 0}, {p4, Vector(0, 1, 0), white, 0}, {p3, Vector(1, 1, 0), white, 0},
            {p1, Vector(0, 0, 0), white, 0}, {p3, Vector(1, 1, 0), white, 0}, {p2, Vector(1, 0, 0), white, 0}
        };

        // Color: blend the offscreen image over the cleared frame
        DrawCommand::State state = renderer.makeDrawState(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
        state.stencilMode = 0;
        state.texture = colorDescriptorSet;
        state.setModulateColor(modulateColor);
        state.pipeline = renderer.getPipeline(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, false, false, BlendFunc::SrcAlpha, false, 0);
        renderer.getBatchRenderer().draw(state, vertices, 6);

        // Depth: copy the offscreen depth so that dynamic objects are occluded by the static walls
        DrawCommand::State depthState = renderer.makeDrawState(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
        depthState.stencilMode = 0;
        depthState.texture = depthDescriptorSet;
        VkRenderPass rp = renderer.getMainRenderPass().getHandle();
        VkDescriptorSetLayout texLayout = renderer.getTextureDescriptorSetLayout();
        depthState.pipeline = renderer.getPipelineCache().getOrCreatePipeline(
            std::hash<std::string>{}("depth_blit"), [&]() {
                return PipelinePresets::createDepthBlitPipeline(renderer.getDevice(), rp, texLayout);
            });
        renderer.getBatchRenderer().draw(depthState, vertices, 6);
    }

    void VulkanRendererTarget::resize(Int32 newWidth, Int32 newHeight) {
        if (static_cast<uint32_t>(newWidth) == width && static_cast<uint32_t>(newHeight) == height) {
            return;
        }

        if (renderer.isInitialized()) {
            renderer.getDevice().waitIdle();
        }

        destroyResources();
        width = static_cast<uint32_t>(newWidth);
        height = static_cast<uint32_t>(newHeight);
        createResources();
    }

} // namespace Duel6
