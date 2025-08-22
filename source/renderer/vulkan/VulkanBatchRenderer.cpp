#include "VulkanBatchRenderer.h"
#include "VulkanDevice.h"
#include "VulkanMemory.h"
#include "../../Exception.h"
#include <algorithm>
#include <cstring>

namespace Duel6 {

    VulkanBatchRenderer::VulkanBatchRenderer(const VulkanDevice& device,
                                           VulkanMemoryManager& memoryManager,
                                           Console& console)
        : device(device)
        , memoryManager(memoryManager)
        , console(console)
        , vertexBuffer(console)
        , indexBuffer(console, VK_INDEX_TYPE_UINT16)
        , vertexBufferOffset(0)
        , indexBufferOffset(0)
        , currentFrame(0) {
        
        // Initialize vertex buffer with reasonable size (1MB)
        vertexBuffer.create(device, memoryManager, 1024 * 1024, false);
        
        // Initialize index buffer with reasonable size (256KB)
        indexBuffer.create(device, memoryManager, 256 * 1024, false);
        
        stats.reset();
    }

    VulkanBatchRenderer::~VulkanBatchRenderer() {
        clear();
    }

    void VulkanBatchRenderer::beginFrame(uint32_t frameIndex) {
        currentFrame = frameIndex % MAX_FRAMES_IN_FLIGHT;
        vertexBufferOffset = 0;
        indexBufferOffset = 0;
        stats.reset();
        
        // Mark all batches as dirty for the new frame
        for (auto& pair : batches) {
            pair.second->dirty = true;
            pair.second->commands.clear();
        }
    }

    void VulkanBatchRenderer::endFrame() {
        // Reset buffer offsets for next frame
        vertexBufferOffset = 0;
        indexBufferOffset = 0;
    }

    void VulkanBatchRenderer::addDrawCommand(VulkanPipeline* pipeline, const DrawCommand& command) {
        if (!pipeline) {
            D6_THROW(Exception, "Pipeline cannot be null");
        }
        
        RenderBatch* batch = getBatch(pipeline);
        batch->commands.push_back(command);
        batch->dirty = true;
        
        // Update statistics
        stats.drawCalls++;
        if (command.type == DrawCommand::TRIANGLES || command.type == DrawCommand::TRIANGLE_STRIP) {
            if (command.indices) {
                stats.triangles += command.indexCount / 3;
            } else {
                stats.triangles += command.vertexCount / 3;
            }
        }
    }

    void VulkanBatchRenderer::flush(VkCommandBuffer commandBuffer, VkRenderPass renderPass) {
        if (batches.empty()) {
            return;
        }
        
        // Begin render pass
        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPass;
        
        stats.batches = 0;
        
        // Process each batch
        for (auto& pair : batches) {
            RenderBatch& batch = *pair.second;
            
            if (batch.commands.empty()) {
                continue;
            }
            
            // Update uniform buffer if dirty
            if (batch.dirty) {
                updateUniformBuffer(batch);
                batch.dirty = false;
            }
            
            // Bind pipeline
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, batch.pipeline->getPipeline());
            
            // Record draw commands for this batch
            recordDrawCommands(commandBuffer, batch);
            
            stats.batches++;
        }
    }

    void VulkanBatchRenderer::reserveVertexData(size_t size) {
        ensureVertexBufferCapacity(vertexBufferOffset + size);
    }

    void VulkanBatchRenderer::reserveIndexData(size_t size) {
        ensureIndexBufferCapacity(indexBufferOffset + size);
    }

    void VulkanBatchRenderer::clear() {
        batches.clear();
        vertexBufferOffset = 0;
        indexBufferOffset = 0;
        stats.reset();
    }

    bool VulkanBatchRenderer::isEmpty() const {
        for (const auto& pair : batches) {
            if (!pair.second->commands.empty()) {
                return false;
            }
        }
        return true;
    }

    RenderBatch* VulkanBatchRenderer::getBatch(VulkanPipeline* pipeline) {
        auto it = batches.find(pipeline);
        if (it == batches.end()) {
            // Create new batch
            auto batch = std::make_unique<RenderBatch>(pipeline);
            
            RenderBatch* batchPtr = batch.get();
            batches[pipeline] = std::move(batch);
            return batchPtr;
        }
        
        return it->second.get();
    }

    void VulkanBatchRenderer::updateUniformBuffer(RenderBatch& batch) {
        // TODO: Implement uniform buffer updates when uniform buffer management is ready
        // For now, we'll skip uniform buffer updates
    }

    void VulkanBatchRenderer::recordDrawCommands(VkCommandBuffer commandBuffer, const RenderBatch& batch) {
        // Bind vertex buffer
        VkBuffer vertexBuffers[] = { vertexBuffer.getBuffer().buffer };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
        
        // Bind descriptor sets
        if (batch.descriptorSet != VK_NULL_HANDLE) {
            VkDescriptorSet descriptorSets[] = { batch.descriptorSet };
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                   batch.pipeline->getLayout(), 0, 1, descriptorSets, 0, nullptr);
        }
        
        // Record individual draw commands
        for (const DrawCommand& cmd : batch.commands) {
            if (cmd.vertices && cmd.vertexDataSize > 0) {
                // Copy vertex data to buffer
                ensureVertexBufferCapacity(vertexBufferOffset + cmd.vertexDataSize);
                void* mappedData = vertexBuffer.map(device, memoryManager);
                if (mappedData) {
                    std::memcpy(static_cast<char*>(mappedData) + vertexBufferOffset, cmd.vertices, cmd.vertexDataSize);
                    vertexBuffer.unmap(device, memoryManager);
                }
                
                if (cmd.indices && cmd.indexCount > 0) {
                    // Indexed drawing
                    size_t indexDataSize = cmd.indexCount * sizeof(uint16_t);
                    ensureIndexBufferCapacity(indexBufferOffset + indexDataSize);
                    
                    void* indexMappedData = indexBuffer.map(device, memoryManager);
                    if (indexMappedData) {
                        std::memcpy(static_cast<char*>(indexMappedData) + indexBufferOffset, cmd.indices, indexDataSize);
                        indexBuffer.unmap(device, memoryManager);
                    }
                    
                    vkCmdBindIndexBuffer(commandBuffer, indexBuffer.getBuffer().buffer, 
                                        indexBufferOffset, VK_INDEX_TYPE_UINT16);
                    vkCmdDrawIndexed(commandBuffer, cmd.indexCount, cmd.instanceCount,
                                    0, vertexBufferOffset / sizeof(Float32), cmd.firstInstance);
                    
                    indexBufferOffset += indexDataSize;
                } else {
                    // Non-indexed drawing
                    vkCmdDraw(commandBuffer, cmd.vertexCount, cmd.instanceCount,
                             cmd.firstVertex, cmd.firstInstance);
                }
                
                vertexBufferOffset += cmd.vertexDataSize;
            }
        }
    }

    VkPrimitiveTopology VulkanBatchRenderer::getVulkanTopology(DrawCommand::Type type) {
        switch (type) {
            case DrawCommand::TRIANGLES:
                return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            case DrawCommand::TRIANGLE_STRIP:
                return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
            case DrawCommand::LINES:
                return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
            case DrawCommand::LINE_STRIP:
                return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
            case DrawCommand::POINTS:
                return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
            default:
                return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        }
    }

    void VulkanBatchRenderer::ensureVertexBufferCapacity(size_t requiredSize) {
        if (requiredSize > vertexBuffer.getCapacity()) {
            // Grow buffer by 50% or to required size, whichever is larger
            size_t newSize = std::max(requiredSize, static_cast<size_t>(vertexBuffer.getCapacity() * 3 / 2));
            
            // Create new buffer
            VulkanVertexBuffer newBuffer(console);
            newBuffer.create(device, memoryManager, newSize, false);
            
            // Copy existing data if any
            if (vertexBufferOffset > 0) {
                // TODO: Implement buffer copy operation
                // For now, we'll lose the data and start fresh
            }
            
            vertexBuffer = std::move(newBuffer);
            vertexBufferOffset = 0;
        }
    }

    void VulkanBatchRenderer::ensureIndexBufferCapacity(size_t requiredSize) {
        if (requiredSize > indexBuffer.getCapacity()) {
            // Grow buffer by 50% or to required size, whichever is larger
            size_t newSize = std::max(requiredSize, static_cast<size_t>(indexBuffer.getCapacity() * 3 / 2));
            
            // Create new buffer
            VulkanIndexBuffer newBuffer(console, VK_INDEX_TYPE_UINT16);
            newBuffer.create(device, memoryManager, newSize, false);
            
            // Copy existing data if any
            if (indexBufferOffset > 0) {
                // TODO: Implement buffer copy operation
                // For now, we'll lose the data and start fresh
            }
            
            indexBuffer = std::move(newBuffer);
            indexBufferOffset = 0;
        }
    }

}
