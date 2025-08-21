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

#include "VulkanTexture.h"
#include "VulkanContext.h"
#include "../../Format.h"
#include <algorithm>
#include <cstring>

namespace Duel6 {
    VulkanTexture::VulkanTexture(Console& console)
        : sampler(VK_NULL_HANDLE), descriptorSet(VK_NULL_HANDLE), width(0), height(0), 
          format(VK_FORMAT_UNDEFINED), isValid(false), console(&console) {
    }

    VulkanTexture::~VulkanTexture() {
        // Resources should be explicitly destroyed before destructor
        if (isValid) {
            console->printLine("Warning: VulkanTexture destroyed without explicit cleanup");
        }
    }

    VulkanTexture::VulkanTexture(VulkanTexture&& other) noexcept
        : image(std::move(other.image)), sampler(other.sampler), descriptorSet(other.descriptorSet),
          width(other.width), height(other.height), format(other.format), 
          isValid(other.isValid), console(other.console) {
        other.sampler = VK_NULL_HANDLE;
        other.descriptorSet = VK_NULL_HANDLE;
        other.width = 0;
        other.height = 0;
        other.format = VK_FORMAT_UNDEFINED;
        other.isValid = false;
    }

    VulkanTexture& VulkanTexture::operator=(VulkanTexture&& other) noexcept {
        if (this != &other) {
            image = std::move(other.image);
            sampler = other.sampler;
            descriptorSet = other.descriptorSet;
            width = other.width;
            height = other.height;
            format = other.format;
            isValid = other.isValid;
            console = other.console;

            other.sampler = VK_NULL_HANDLE;
            other.descriptorSet = VK_NULL_HANDLE;
            other.width = 0;
            other.height = 0;
            other.format = VK_FORMAT_UNDEFINED;
            other.isValid = false;
        }
        return *this;
    }

    void VulkanTexture::create(const VulkanDevice& device, VulkanMemoryManager& memoryManager,
                              const Image& sourceImage, TextureFilter filter, bool clamp) {
        if (isValid) {
            destroy(device, memoryManager);
        }

        width = sourceImage.getWidth();
        height = sourceImage.getHeight();
        format = selectFormat(sourceImage);

        // Create Vulkan image
        image = memoryManager.createImage(device, width, height, format, VK_IMAGE_TILING_OPTIMAL,
                                         VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                                         VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        // Create sampler
        createSampler(device, filter, clamp);

        // Upload image data
        uploadImageData(device, memoryManager, sourceImage);

        isValid = true;
    }

    void VulkanTexture::create(const VulkanDevice& device, VulkanMemoryManager& memoryManager,
                              uint32_t w, uint32_t h, VkFormat fmt, TextureFilter filter, bool clamp) {
        if (isValid) {
            destroy(device, memoryManager);
        }

        width = w;
        height = h;
        format = fmt;

        // Create Vulkan image
        image = memoryManager.createImage(device, width, height, format, VK_IMAGE_TILING_OPTIMAL,
                                         VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                                         VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        // Create sampler
        createSampler(device, filter, clamp);

        isValid = true;
    }

    void VulkanTexture::destroy(const VulkanDevice& device, VulkanMemoryManager& memoryManager) {
        if (!isValid) {
            return;
        }

        // Sampler cleanup
        if (sampler != VK_NULL_HANDLE) {
            vkDestroySampler(device.getLogicalDevice(), sampler, nullptr);
            sampler = VK_NULL_HANDLE;
        }

        // Image cleanup
        memoryManager.destroyImage(device, image);

        descriptorSet = VK_NULL_HANDLE; // Managed by descriptor pool
        width = 0;
        height = 0;
        format = VK_FORMAT_UNDEFINED;
        isValid = false;
    }

    void VulkanTexture::updateData(const VulkanDevice& device, VulkanMemoryManager& memoryManager, const Image& sourceImage) {
        if (!isValid) {
            D6_THROW(VulkanException, "Cannot update invalid texture");
        }

        if (sourceImage.getWidth() != width || sourceImage.getHeight() != height) {
            D6_THROW(VulkanException, "Image dimensions don't match texture dimensions");
        }

        uploadImageData(device, memoryManager, sourceImage);
    }

    void VulkanTexture::updateSubData(const VulkanDevice& device, VulkanMemoryManager& memoryManager,
                                     uint32_t x, uint32_t y, uint32_t w, uint32_t h, const void* data) {
        if (!isValid) {
            D6_THROW(VulkanException, "Cannot update invalid texture");
        }

        if (x + w > width || y + h > height) {
            D6_THROW(VulkanException, "Sub-image update out of bounds");
        }

        // Create staging buffer for the sub-image data
        size_t dataSize = w * h * 4; // Assuming RGBA format
        VulkanBuffer stagingBuffer = memoryManager.createStagingBuffer(device, dataSize);

        // Copy data to staging buffer
        memoryManager.uploadBufferData(device, stagingBuffer, data, dataSize);

        // Transition image layout for transfer
        memoryManager.transitionImageLayout(device, image.image, format, 
                                           VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        // Copy staging buffer to image
        VkCommandBuffer commandBuffer = device.beginSingleTimeCommands();

        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = {static_cast<int32_t>(x), static_cast<int32_t>(y), 0};
        region.imageExtent = {w, h, 1};

        vkCmdCopyBufferToImage(commandBuffer, stagingBuffer.buffer, image.image, 
                              VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        device.endSingleTimeCommands(commandBuffer);

        // Transition back to shader read layout
        memoryManager.transitionImageLayout(device, image.image, format,
                                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        // Cleanup staging buffer
        memoryManager.destroyBuffer(device, stagingBuffer);
    }

    void VulkanTexture::createSampler(const VulkanDevice& device, TextureFilter filter, bool clamp) {
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = toVulkanFilter(filter);
        samplerInfo.minFilter = toVulkanFilter(filter);
        samplerInfo.addressModeU = toVulkanAddressMode(clamp);
        samplerInfo.addressModeV = toVulkanAddressMode(clamp);
        samplerInfo.addressModeW = toVulkanAddressMode(clamp);
        samplerInfo.anisotropyEnable = VK_FALSE;
        samplerInfo.maxAnisotropy = 1.0f;
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = 0.0f;

        VkResult result = vkCreateSampler(device.getLogicalDevice(), &samplerInfo, nullptr, &sampler);
        if (result != VK_SUCCESS) {
            D6_THROW(VulkanException, Format("Failed to create texture sampler, error: {0}") << result);
        }
    }

    VkFormat VulkanTexture::selectFormat(const Image& sourceImage) const {
        // For now, always use RGBA8 format
        // In the future, we might want to select based on image properties
        return VK_FORMAT_R8G8B8A8_UNORM;
    }

    void VulkanTexture::uploadImageData(const VulkanDevice& device, VulkanMemoryManager& memoryManager, const Image& sourceImage) {
        // Convert image data to RGBA format
        std::vector<uint8_t> pixelData;
        pixelData.reserve(width * height * 4);

        for (uint32_t y = 0; y < height; ++y) {
            for (uint32_t x = 0; x < width; ++x) {
                Color pixel = sourceImage.at(y * width + x);
                pixelData.push_back(static_cast<uint8_t>(pixel.getRed() * 255));
                pixelData.push_back(static_cast<uint8_t>(pixel.getGreen() * 255));
                pixelData.push_back(static_cast<uint8_t>(pixel.getBlue() * 255));
                pixelData.push_back(static_cast<uint8_t>(pixel.getAlpha() * 255));
            }
        }

        size_t imageSize = pixelData.size();
        VulkanBuffer stagingBuffer = memoryManager.createStagingBuffer(device, imageSize);

        // Copy pixel data to staging buffer
        memoryManager.uploadBufferData(device, stagingBuffer, pixelData.data(), imageSize);

        // Transition image layout and copy from staging buffer
        memoryManager.transitionImageLayout(device, image.image, format,
                                           VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        memoryManager.copyBufferToImage(device, stagingBuffer.buffer, image.image, width, height);

        memoryManager.transitionImageLayout(device, image.image, format,
                                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        // Cleanup staging buffer
        memoryManager.destroyBuffer(device, stagingBuffer);
    }
}