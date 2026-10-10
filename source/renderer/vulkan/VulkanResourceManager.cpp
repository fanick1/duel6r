#include "VulkanResourceManager.h"

namespace Duel6 {

    VulkanResourceManager::~VulkanResourceManager() {
        if (!textures.empty()) {
            console->printLine("Warning: VulkanResourceManager destroyed with textures still allocated");
        }
    }

    void VulkanResourceManager::initialize(VulkanMemoryManager* memMgr, VulkanSamplerManager* samplerMgr, VulkanDescriptorManager* descMgr) {
        memoryManager = memMgr;
        samplerManager = samplerMgr;
        descriptorManager = descMgr;
        console->printLine("...Vulkan resource manager initialized");
    }

    void VulkanResourceManager::cleanup(const VulkanDevice& device) {
        for (auto& pair : textures) {
            destroy(device, pair.second);
        }
        textures.clear();
        pendingDeletes.clear();
        deleteQueue.clear();
        nextTextureId = 1;
        console->printLine("...Vulkan resource manager cleaned up");
    }

    TextureId VulkanResourceManager::createTexture(const VulkanDevice& device, const Image& image, TextureFilter filter, bool clamp) {
        Entry entry;
        entry.texture = std::make_unique<VulkanTexture>(*console);
        entry.texture->create(device, *memoryManager, image);

        VkDescriptorSetLayout layout = descriptorManager->getTextureLayout(device);
        entry.descriptorSet = descriptorManager->allocateDescriptorSet(device, layout);
        VkSampler sampler = samplerManager->getSampler(device, filter, clamp);
        descriptorManager->updateTextureDescriptorSet(device, entry.descriptorSet, 0, entry.texture->getImage().view, sampler);

        TextureId id = nextTextureId++;
        textures[id] = std::move(entry);
        return id;
    }

    void VulkanResourceManager::deleteTexture(TextureId id) {
        if (textures.count(id) != 0) {
            pendingDeletes.push_back(id);
        }
    }

    VkDescriptorSet VulkanResourceManager::getTextureDescriptorSet(TextureId id) const {
        auto it = textures.find(id);
        return it != textures.end() ? it->second.descriptorSet : VK_NULL_HANDLE;
    }

    void VulkanResourceManager::processDeletes(const VulkanDevice& device) {
        // The frame just submitted and the other frame in flight may still read these textures,
        // so they are retired DELETE_DELAY_FRAMES frames later, once their fences have been waited on
        deleteQueue.push_back(std::move(pendingDeletes));
        pendingDeletes.clear();

        while (deleteQueue.size() > DELETE_DELAY_FRAMES) {
            for (TextureId id : deleteQueue.front()) {
                auto it = textures.find(id);
                if (it != textures.end()) {
                    destroy(device, it->second);
                    textures.erase(it);
                }
            }
            deleteQueue.pop_front();
        }
    }

    void VulkanResourceManager::destroy(const VulkanDevice& device, Entry& entry) {
        descriptorManager->freeDescriptorSet(device, entry.descriptorSet);
        entry.texture->destroy(device, *memoryManager);
    }
}
