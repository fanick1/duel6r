#include "VulkanBatchRenderer.h"
#include "VulkanDevice.h"
#include "VulkanMemory.h"
#include "../../Exception.h"
#include <algorithm>
#include <cstring>
#include <iostream>

namespace Duel6 {

    VulkanBatchRenderer::VulkanBatchRenderer(const VulkanDevice& device,
                                           VulkanMemoryManager& memoryManager,
                                           Console& console)
        : device(device)
        , memoryManager(memoryManager)
        , console(console)
        , vertexBuffers{VulkanVertexBuffer(console), VulkanVertexBuffer(console)}
        , indexBuffers{VulkanIndexBuffer(console, VK_INDEX_TYPE_UINT16), VulkanIndexBuffer(console, VK_INDEX_TYPE_UINT16)}
        , vertexBufferOffset(0)
        , indexBufferOffset(0)
        , currentFrame(0)
        , lastPipeline(nullptr) {
        
        // Initialize per-frame vertex and index buffers
        for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vertexBuffers[i].create(device, memoryManager, 1024 * 1024, false);
            indexBuffers[i].create(device, memoryManager, 256 * 1024, false);
        }
        
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
        
        // Clear all batch state for the new frame
        orderedBatches.clear();
        lastPipeline = nullptr;
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
        if (orderedBatches.empty()) {
            return;
        }
        
        // Safety check - don't try to record if command buffer is null
        if (commandBuffer == VK_NULL_HANDLE) {
            std::cerr << "[WARNING] VulkanBatchRenderer::flush called with null command buffer, skipping" << std::endl;
            return;
        }
        
        // Count total commands
        size_t totalCommands = 0;
        for (const auto& batch : orderedBatches) {
            totalCommands += batch->commands.size();
        }
        if (totalCommands == 0) {
            return;
        }
        
        // Set dynamic viewport (negative height flips Y to match OpenGL conventions)
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = static_cast<float>(viewportHeight);
        viewport.width = static_cast<float>(viewportWidth);
        viewport.height = -static_cast<float>(viewportHeight);
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
        
        // Process each batch IN ORDER (preserves stencil/state ordering)
        for (auto& batch : orderedBatches) {
            if (batch->commands.empty()) {
                continue;
            }
            
            // Update uniform buffer if dirty
            if (batch->dirty) {
                updateUniformBuffer(*batch);
                batch->dirty = false;
            }
            
            // Bind pipeline
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, batch->pipeline->getPipeline());
            
            // Set default line width for line-topology pipelines (dynamic state)
            vkCmdSetLineWidth(commandBuffer, 1.0f);
            
            // Record draw commands for this batch
            recordDrawCommands(commandBuffer, *batch);
            
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
        orderedBatches.clear();
        lastPipeline = nullptr;
        vertexBufferOffset = 0;
        indexBufferOffset = 0;
        stats.reset();
    }

    bool VulkanBatchRenderer::isEmpty() const {
        for (const auto& batch : orderedBatches) {
            if (!batch->commands.empty()) {
                return false;
            }
        }
        return true;
    }

    RenderBatch* VulkanBatchRenderer::getBatch(VulkanPipeline* pipeline) {
        // If the last batch uses the same pipeline, reuse it (preserves ordering)
        if (!orderedBatches.empty() && orderedBatches.back()->pipeline == pipeline) {
            return orderedBatches.back().get();
        }
        
        // Create a new batch to preserve ordering
        auto batch = std::make_unique<RenderBatch>(pipeline);
        RenderBatch* batchPtr = batch.get();
        orderedBatches.push_back(std::move(batch));
        lastPipeline = pipeline;
        return batchPtr;
    }

    void VulkanBatchRenderer::updateUniformBuffer(RenderBatch& batch) {
        // TODO: Implement uniform buffer updates when uniform buffer management is ready
        // For now, we'll skip uniform buffer updates
    }

    void VulkanBatchRenderer::recordDrawCommands(VkCommandBuffer commandBuffer, const RenderBatch& batch) {
        // Bind the current frame's vertex buffer
        VkBuffer vbHandles[] = { vertexBuffers[currentFrame].getBuffer().buffer };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vbHandles, offsets);
        
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
        
        // Ensure current frame's vertex buffer is large enough for all data
        ensureVertexBufferCapacity(vertexBufferOffset + totalVertexDataSize);
        
        // Map current frame's vertex buffer ONCE for all draw commands in this batch
        void* mappedData = vertexBuffers[currentFrame].map(device, memoryManager);
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
        vertexBuffers[currentFrame].unmap(device, memoryManager);
        
        // Second pass: record draw commands
        size_t currentOffset = batchStartOffset;
        for (const DrawCommand& cmd : batch.commands) {
            // Handle special clear-stencil command
            if (cmd.stencilMode == 4) {
                VkClearAttachment clearAtt{};
                clearAtt.aspectMask = VK_IMAGE_ASPECT_STENCIL_BIT;
                clearAtt.clearValue.depthStencil = {1.0f, 0};
                VkClearRect clearRect{};
                clearRect.rect.offset = {0, 0};
                clearRect.rect.extent = {4096, 4096}; // Large enough for any framebuffer
                clearRect.baseArrayLayer = 0;
                clearRect.layerCount = 1;
                vkCmdClearAttachments(commandBuffer, 1, &clearAtt, 1, &clearRect);
                continue;
            }

            // Set dynamic stencil state based on stencil mode
            if (cmd.stencilMode > 0 && cmd.stencilMode <= 3) {
                switch (cmd.stencilMode) {
                    case 1: // WRITE: reference=1, compare mask=all, write mask=all
                        vkCmdSetStencilCompareMask(commandBuffer, VK_STENCIL_FACE_FRONT_AND_BACK, 0xFFFFFFFF);
                        vkCmdSetStencilWriteMask(commandBuffer, VK_STENCIL_FACE_FRONT_AND_BACK, 0xFFFFFFFF);
                        vkCmdSetStencilReference(commandBuffer, VK_STENCIL_FACE_FRONT_AND_BACK, 1);
                        break;
                    case 2: // TEST: reference=0, compare mask=all, write mask=0
                        vkCmdSetStencilCompareMask(commandBuffer, VK_STENCIL_FACE_FRONT_AND_BACK, 0xFFFFFFFF);
                        vkCmdSetStencilWriteMask(commandBuffer, VK_STENCIL_FACE_FRONT_AND_BACK, 0);
                        vkCmdSetStencilReference(commandBuffer, VK_STENCIL_FACE_FRONT_AND_BACK, 0);
                        break;
                    case 3: // ERASE: reference=0, compare mask=all, write mask=all
                        vkCmdSetStencilCompareMask(commandBuffer, VK_STENCIL_FACE_FRONT_AND_BACK, 0xFFFFFFFF);
                        vkCmdSetStencilWriteMask(commandBuffer, VK_STENCIL_FACE_FRONT_AND_BACK, 0xFFFFFFFF);
                        vkCmdSetStencilReference(commandBuffer, VK_STENCIL_FACE_FRONT_AND_BACK, 0);
                        break;
                }
            }

            if (cmd.vertices && cmd.vertexDataSize > 0) {
                // Push MVP matrix using push constants
                Matrix mvp = cmd.projectionMatrix * cmd.viewMatrix * cmd.modelMatrix;
                
                if (cmd.textureDescriptorSet != VK_NULL_HANDLE) {
                    // Textured draw: bind descriptor set and push extended push constants
                    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                           batch.pipeline->getLayout(), 0, 1,
                                           &cmd.textureDescriptorSet, 0, nullptr);

                    // Push vertex stage: mat4 MVP (bytes 0-63)
                    vkCmdPushConstants(commandBuffer, batch.pipeline->getLayout(),
                                       VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(float) * 16,
                                       mvp.getStorage());

                    // Push fragment stage: vec4 modulateColor (bytes 64-79) + uint alphaTest (bytes 80-83)
                    struct { float color[4]; uint32_t alphaTest; } materialData;
                    materialData.color[0] = cmd.modulateColor[0];
                    materialData.color[1] = cmd.modulateColor[1];
                    materialData.color[2] = cmd.modulateColor[2];
                    materialData.color[3] = cmd.modulateColor[3];
                    materialData.alphaTest = cmd.alphaTest;

                    vkCmdPushConstants(commandBuffer, batch.pipeline->getLayout(),
                                       VK_SHADER_STAGE_FRAGMENT_BIT, 64, 20,
                                       &materialData);
                } else {
                    // Color-only draw: push MVP only
                    vkCmdPushConstants(commandBuffer, batch.pipeline->getLayout(),
                                       VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(float) * 16,
                                       mvp.getStorage());
                }
                
                // Calculate vertex size and first vertex index
                size_t vertexSize = cmd.vertexDataSize / cmd.vertexCount;
                uint32_t firstVertex = static_cast<uint32_t>(currentOffset / vertexSize);
                
                if (cmd.indices && cmd.indexCount > 0) {
                    // Indexed drawing
                    size_t indexDataSize = cmd.indexCount * sizeof(uint16_t);
                    ensureIndexBufferCapacity(indexBufferOffset + indexDataSize);
                    
                    void* indexMappedData = indexBuffers[currentFrame].map(device, memoryManager);
                    if (indexMappedData) {
                        std::memcpy(static_cast<char*>(indexMappedData) + indexBufferOffset, cmd.indices, indexDataSize);
                        indexBuffers[currentFrame].unmap(device, memoryManager);
                    }
                    
                    vkCmdBindIndexBuffer(commandBuffer, indexBuffers[currentFrame].getBuffer().buffer, 
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
        VulkanVertexBuffer& vb = vertexBuffers[currentFrame];
        if (requiredSize > vb.getCapacity()) {
            // Grow buffer by 50% or to required size, whichever is larger
            size_t newSize = std::max(requiredSize, static_cast<size_t>(vb.getCapacity() * 3 / 2));
            
            // Create new buffer
            VulkanVertexBuffer newBuffer(console);
            newBuffer.create(device, memoryManager, newSize, false);
            
            // Copy existing data if any
            if (vertexBufferOffset > 0) {
                // TODO: Implement buffer copy operation
                // For now, we'll lose the data and start fresh
            }
            
            vb = std::move(newBuffer);
            vertexBufferOffset = 0;
        }
    }

    void VulkanBatchRenderer::ensureIndexBufferCapacity(size_t requiredSize) {
        VulkanIndexBuffer& ib = indexBuffers[currentFrame];
        if (requiredSize > ib.getCapacity()) {
            // Grow buffer by 50% or to required size, whichever is larger
            size_t newSize = std::max(requiredSize, static_cast<size_t>(ib.getCapacity() * 3 / 2));
            
            // Create new buffer
            VulkanIndexBuffer newBuffer(console, VK_INDEX_TYPE_UINT16);
            newBuffer.create(device, memoryManager, newSize, false);
            
            // Copy existing data if any
            if (indexBufferOffset > 0) {
                // TODO: Implement buffer copy operation
                // For now, we'll lose the data and start fresh
            }
            
            ib = std::move(newBuffer);
            indexBufferOffset = 0;
        }
    }

}
