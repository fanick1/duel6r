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
        
        // Clear all batch state for the new frame, including vertex data storage.
        // This also handles the case where pipeline pointers may have been
        // invalidated by swapchain recreation between frames.
        for (auto& pair : batches) {
            pair.second->dirty = true;
            pair.second->commands.clear();
            pair.second->vertexDataStorage.clear();
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
        
        // Copy vertex data to persistent storage
        DrawCommand cmd = command;
        if (command.vertices && command.vertexDataSize > 0) {
            batch->vertexDataStorage.emplace_back(command.vertexDataSize);
            std::memcpy(batch->vertexDataStorage.back().data(), command.vertices, command.vertexDataSize);
            cmd.vertices = batch->vertexDataStorage.back().data();
        }
        
        batch->commands.push_back(cmd);
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

    void VulkanBatchRenderer::flush(VkCommandBuffer commandBuffer, VkRenderPass renderPass, 
                                     uint32_t viewportWidth, uint32_t viewportHeight,
                                     bool scissorEnabled, int32_t scissorX, int32_t scissorY,
                                     int32_t scissorWidth, int32_t scissorHeight) {
        if (batches.empty()) {
            return;
        }
        
        // Safety check - don't try to record if command buffer is null
        if (commandBuffer == VK_NULL_HANDLE) {
            std::cerr << "[WARNING] VulkanBatchRenderer::flush called with null command buffer, skipping" << std::endl;
            return;
        }
        
        // Count total commands
        size_t totalCommands = 0;
        for (const auto& pair : batches) {
            totalCommands += pair.second->commands.size();
        }
        if (totalCommands == 0) {
            return;
        }
        
        // Set dynamic viewport
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(viewportWidth);
        viewport.height = static_cast<float>(viewportHeight);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
        
        // Set dynamic scissor
        VkRect2D scissor{};
        if (scissorEnabled && scissorWidth > 0 && scissorHeight > 0) {
            // Clamp scissor to viewport bounds
            int32_t clampedX = std::max(0, scissorX);
            int32_t clampedY = std::max(0, scissorY);
            int32_t clampedWidth = std::min(scissorWidth, static_cast<int32_t>(viewportWidth) - clampedX);
            int32_t clampedHeight = std::min(scissorHeight, static_cast<int32_t>(viewportHeight) - clampedY);
            
            scissor.offset = {clampedX, clampedY};
            scissor.extent = {static_cast<uint32_t>(std::max(0, clampedWidth)), 
                              static_cast<uint32_t>(std::max(0, clampedHeight))};
        } else {
            scissor.offset = {0, 0};
            scissor.extent = {viewportWidth, viewportHeight};
        }
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
        
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
        
        // Calculate total vertex data size for this batch
        size_t totalVertexDataSize = 0;
        for (const DrawCommand& cmd : batch.commands) {
            if (cmd.vertices && cmd.vertexDataSize > 0) {
                totalVertexDataSize += cmd.vertexDataSize;
            }
        }
        
        if (totalVertexDataSize == 0) {
            return;
        }
        
        // Ensure vertex buffer is large enough for all data
        ensureVertexBufferCapacity(vertexBufferOffset + totalVertexDataSize);
        
        // Map vertex buffer ONCE for all draw commands in this batch
        void* mappedData = vertexBuffer.map(device, memoryManager);
        if (!mappedData) {
            return;
        }
        
        // First pass: copy all vertex data into the buffer
        size_t batchStartOffset = vertexBufferOffset;
        for (const DrawCommand& cmd : batch.commands) {
            if (cmd.vertices && cmd.vertexDataSize > 0) {
                std::memcpy(static_cast<char*>(mappedData) + vertexBufferOffset, cmd.vertices, cmd.vertexDataSize);
                vertexBufferOffset += cmd.vertexDataSize;
            }
        }
        
        // Unmap ONCE after all data is written
        vertexBuffer.unmap(device, memoryManager);
        
        // Second pass: record draw commands
        size_t currentOffset = batchStartOffset;
        for (const DrawCommand& cmd : batch.commands) {
            if (cmd.vertices && cmd.vertexDataSize > 0) {
                // Push MVP matrix using push constants
                Matrix mvp = cmd.projectionMatrix * cmd.viewMatrix * cmd.modelMatrix;
                
                vkCmdPushConstants(commandBuffer, batch.pipeline->getLayout(),
                                   VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(float) * 16, mvp.getStorage());
                
                // Calculate vertex size and first vertex index
                size_t vertexSize = cmd.vertexDataSize / cmd.vertexCount;
                uint32_t firstVertex = static_cast<uint32_t>(currentOffset / vertexSize);
                
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
                                    0, firstVertex, cmd.firstInstance);
                    
                    indexBufferOffset += indexDataSize;
                } else {
                    // Non-indexed drawing
                    vkCmdDraw(commandBuffer, cmd.vertexCount, cmd.instanceCount,
                             firstVertex, cmd.firstInstance);
                }
                
                currentOffset += cmd.vertexDataSize;
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
