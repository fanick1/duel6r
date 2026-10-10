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

namespace Duel6 {
    VulkanTexture::VulkanTexture(Console& console)
        : width(0), height(0), depth(1), format(VK_FORMAT_UNDEFINED), isValid(false), console(&console) {
    }

    VulkanTexture::~VulkanTexture() {
        // Resources should be explicitly destroyed before destructor
        if (isValid) {
            console->printLine("Warning: VulkanTexture destroyed without explicit cleanup");
        }
    }

    void VulkanTexture::create(const VulkanDevice& device, VulkanMemoryManager& memoryManager, const Image& sourceImage) {
        if (isValid) {
            destroy(device, memoryManager);
        }

        width = sourceImage.getWidth();
        height = sourceImage.getHeight();
        depth = std::max(static_cast<uint32_t>(sourceImage.getDepth()), 1u);
        format = VK_FORMAT_R8G8B8A8_UNORM;

        // Create Vulkan image (2D array if depth > 1)
        image = memoryManager.createImage(device, width, height, format, VK_IMAGE_TILING_OPTIMAL,
                                         VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                                         VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depth);


        // Upload image data
        uploadImageData(device, memoryManager, sourceImage);

        isValid = true;
    }

    void VulkanTexture::destroy(const VulkanDevice& device, VulkanMemoryManager& memoryManager) {
        if (!isValid) {
            return;
        }

        memoryManager.destroyImage(device, image);

        width = 0;
        height = 0;
        depth = 1;
        format = VK_FORMAT_UNDEFINED;
        isValid = false;
    }

    void VulkanTexture::uploadImageData(const VulkanDevice& device, VulkanMemoryManager& memoryManager, const Image& sourceImage) {
        // Convert image data to RGBA format — all layers stored contiguously
        uint32_t totalPixels = width * height * depth;
        std::vector<uint8_t> pixelData;
        pixelData.reserve(totalPixels * 4);

        for (uint32_t i = 0; i < totalPixels; ++i) {
            Color pixel = sourceImage.at(i);
            pixelData.push_back(pixel.getRed());
            pixelData.push_back(pixel.getGreen());
            pixelData.push_back(pixel.getBlue());
            pixelData.push_back(pixel.getAlpha());
        }

        size_t imageSize = pixelData.size();
        VulkanBuffer stagingBuffer = memoryManager.createStagingBuffer(device, imageSize);

        // Copy pixel data to staging buffer
        memoryManager.uploadBufferData(device, stagingBuffer, pixelData.data(), imageSize);

        // Transition image layout and copy from staging buffer (all layers)
        memoryManager.transitionImageLayout(device, image.image,
                                           VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                           depth);

        memoryManager.copyBufferToImage(device, stagingBuffer.buffer, image.image, width, height, depth);

        memoryManager.transitionImageLayout(device, image.image,
                                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                           depth);

        // Cleanup staging buffer
        memoryManager.destroyBuffer(device, stagingBuffer);
    }
}