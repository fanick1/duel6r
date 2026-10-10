#ifndef DUEL6_RENDERER_VULKANRESOURCEMANAGER_H
#define DUEL6_RENDERER_VULKANRESOURCEMANAGER_H

#include <vulkan/vulkan.h>
#include <memory>
#include <vector>
#include <deque>
#include <unordered_map>
#include "VulkanDevice.h"
#include "VulkanMemory.h"
#include "VulkanTexture.h"
#include "VulkanSampler.h"
#include "VulkanDescriptor.h"
#include "VulkanTypes.h"
#include "../../Image.h"
#include "../../console/Console.h"

namespace Duel6 {

    // Owns the textures the renderer hands out as TextureIds. A deleted texture is destroyed only
    // once every frame that could still be drawing with it has finished.
    class VulkanResourceManager {
    private:
        struct Entry {
            std::unique_ptr<VulkanTexture> texture;
            VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
        };

        static constexpr size_t DELETE_DELAY_FRAMES = 2;  // = frames in flight

        VulkanMemoryManager* memoryManager = nullptr;
        VulkanSamplerManager* samplerManager = nullptr;
        VulkanDescriptorManager* descriptorManager = nullptr;
        Console* console;

        std::unordered_map<TextureId, Entry> textures;
        TextureId nextTextureId = 1;
        std::vector<TextureId> pendingDeletes;           // deleted during the current frame
        std::deque<std::vector<TextureId>> deleteQueue;  // deleted in earlier frames, oldest first

    public:
        explicit VulkanResourceManager(Console& console) : console(&console) {}
        ~VulkanResourceManager();

        VulkanResourceManager(const VulkanResourceManager&) = delete;
        VulkanResourceManager& operator=(const VulkanResourceManager&) = delete;

        void initialize(VulkanMemoryManager* memMgr, VulkanSamplerManager* samplerMgr, VulkanDescriptorManager* descMgr);
        void cleanup(const VulkanDevice& device);

        TextureId createTexture(const VulkanDevice& device, const Image& image, TextureFilter filter, bool clamp);
        void deleteTexture(TextureId id);
        VkDescriptorSet getTextureDescriptorSet(TextureId id) const;

        // Called once per frame after the frame's command buffer was submitted
        void processDeletes(const VulkanDevice& device);

    private:
        void destroy(const VulkanDevice& device, Entry& entry);
    };
}

#endif
