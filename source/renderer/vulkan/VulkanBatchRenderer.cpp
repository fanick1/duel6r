#include "VulkanBatchRenderer.h"
#include "VulkanDevice.h"
#include "VulkanPipeline.h"
#include "../../Color.h"
#include "../../Format.h"
#include "../../console/Console.h"
#include <algorithm>
#include <cstring>

namespace Duel6 {

    namespace {
        // Strips are continuous, so two of them cannot be drawn as one
        bool isListTopology(VkPrimitiveTopology topology) {
            return topology == VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST ||
                   topology == VK_PRIMITIVE_TOPOLOGY_LINE_LIST ||
                   topology == VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
        }

        // Dynamic stencil state of the stencil modes (the stencil ops are baked into the pipelines)
        void setStencilState(VkCommandBuffer commandBuffer, uint32_t mode) {
            uint32_t writeMask = (mode == 2) ? 0 : 0xFFFFFFFF;  // TEST only compares
            uint32_t reference = (mode == 1) ? 1 : 0;           // WRITE marks pixels with 1
            vkCmdSetStencilCompareMask(commandBuffer, VK_STENCIL_FACE_FRONT_AND_BACK, 0xFFFFFFFF);
            vkCmdSetStencilWriteMask(commandBuffer, VK_STENCIL_FACE_FRONT_AND_BACK, writeMask);
            vkCmdSetStencilReference(commandBuffer, VK_STENCIL_FACE_FRONT_AND_BACK, reference);
        }

        // OpenGL rectangle (origin bottom left, y up) -> Vulkan rectangle, clamped to the framebuffer
        VkRect2D toVulkanRect(const Int32 rect[4], uint32_t framebufferWidth, uint32_t framebufferHeight, bool flipY) {
            Int32 fbWidth = static_cast<Int32>(framebufferWidth);
            Int32 fbHeight = static_cast<Int32>(framebufferHeight);
            Int32 left = std::max(rect[0], 0);
            Int32 top = std::max(flipY ? fbHeight - rect[1] - rect[3] : rect[1], 0);
            Int32 right = std::min(rect[0] + rect[2], fbWidth);
            Int32 bottom = std::min(flipY ? fbHeight - rect[1] : rect[1] + rect[3], fbHeight);
            VkRect2D result{};
            result.offset.x = left;
            result.offset.y = top;
            result.extent.width = static_cast<uint32_t>(std::max(right - left, 0));
            result.extent.height = static_cast<uint32_t>(std::max(bottom - top, 0));
            return result;
        }
    }

    void DrawCommand::State::setModulateColor(const Color& color) {
        modulateColor[0] = color.getRed() / 255.0f;
        modulateColor[1] = color.getGreen() / 255.0f;
        modulateColor[2] = color.getBlue() / 255.0f;
        modulateColor[3] = color.getAlpha() / 255.0f;
    }

    bool DrawCommand::State::operator==(const State& other) const {
        return pipeline == other.pipeline &&
               topology == other.topology &&
               texture == other.texture &&
               std::memcmp(modulateColor, other.modulateColor, sizeof(modulateColor)) == 0 &&
               alphaTest == other.alphaTest &&
               time == other.time &&
               stencilMode == other.stencilMode &&
               std::memcmp(viewport, other.viewport, sizeof(viewport)) == 0 &&
               scissorEnabled == other.scissorEnabled &&
               (!scissorEnabled || std::memcmp(scissor, other.scissor, sizeof(scissor)) == 0) &&
               std::memcmp(mvp.getStorage(), other.mvp.getStorage(), sizeof(Float32) * 16) == 0;
    }

    VulkanBatchRenderer::VulkanBatchRenderer(const VulkanDevice& device, VulkanMemoryManager& memoryManager, Console& console)
        : device(device), memoryManager(memoryManager), console(console) {
        for (FrameVertices& frame : frames) {
            frame.buffers.push_back(createVertexBuffer(INITIAL_BUFFER_SIZE));
        }
    }

    VulkanBatchRenderer::~VulkanBatchRenderer() {
        for (FrameVertices& frame : frames) {
            for (VulkanBuffer& buffer : frame.buffers) {
                memoryManager.destroyBuffer(device, buffer);
            }
        }
    }

    VulkanBuffer VulkanBatchRenderer::createVertexBuffer(VkDeviceSize size) {
        VulkanBuffer buffer = memoryManager.createBuffer(device, size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        memoryManager.mapBuffer(device, buffer);  // stays mapped for its whole life
        return buffer;
    }

    void VulkanBatchRenderer::beginFrame(uint32_t frameIndex) {
        currentFrame = frameIndex % MAX_FRAMES_IN_FLIGHT;
        FrameVertices& frame = frames[currentFrame];

        if (frame.buffers.size() > 1) {
            // The last frame in this slot overflowed: replace the chain with one buffer that fits it
            VkDeviceSize size = frame.buffers.front().size;
            while (size < frame.used) {
                size *= 2;
            }
            for (VulkanBuffer& buffer : frame.buffers) {
                memoryManager.destroyBuffer(device, buffer);
            }
            frame.buffers.clear();
            frame.buffers.push_back(createVertexBuffer(size));
            console.printLine(Format("[vulkan] vertex buffer grown to {0} KB") << size / 1024);
        }

        frame.current = 0;
        frame.offset = 0;
        frame.used = 0;
        commands.clear();
    }

    void* VulkanBatchRenderer::allocateVertices(VkDeviceSize size, VkBuffer& buffer, VkDeviceSize& offset) {
        FrameVertices& frame = frames[currentFrame];
        if (frame.offset + size > frame.buffers[frame.current].size) {
            if (frame.current + 1 == frame.buffers.size()) {
                frame.buffers.push_back(createVertexBuffer(std::max(INITIAL_BUFFER_SIZE, size)));
            }
            frame.current++;
            frame.offset = 0;
        }

        VulkanBuffer& current = frame.buffers[frame.current];
        buffer = current.buffer;
        offset = frame.offset;
        frame.offset += size;
        frame.used += size;
        return static_cast<uint8_t*>(current.mapped) + offset;
    }

    bool VulkanBatchRenderer::canMerge(const DrawCommand& last, const DrawCommand& next) const {
        return last.op == DrawCommand::Op::Draw && next.op == DrawCommand::Op::Draw &&
               isListTopology(next.state.topology) &&
               last.vertexBuffer == next.vertexBuffer &&
               last.vertexOffset + VkDeviceSize(last.vertexCount) * sizeof(VulkanVertex) == next.vertexOffset &&
               last.state == next.state;
    }

    void VulkanBatchRenderer::draw(const DrawCommand::State& state, const VulkanVertex* vertices, uint32_t vertexCount) {
        if (vertexCount == 0 || state.pipeline == nullptr || state.texture == VK_NULL_HANDLE ||
            state.viewport[2] <= 0 || state.viewport[3] <= 0) {
            return;
        }

        DrawCommand command;
        command.state = state;
        command.vertexCount = vertexCount;
        VkDeviceSize size = VkDeviceSize(vertexCount) * sizeof(VulkanVertex);
        void* destination = allocateVertices(size, command.vertexBuffer, command.vertexOffset);
        std::memcpy(destination, vertices, size);

        if (!commands.empty() && canMerge(commands.back(), command)) {
            commands.back().vertexCount += vertexCount;
        } else {
            commands.push_back(command);
        }
    }

    void VulkanBatchRenderer::clearStencil() {
        DrawCommand command;
        command.op = DrawCommand::Op::ClearStencil;
        commands.push_back(command);
    }

    void VulkanBatchRenderer::discard() {
        commands.clear();
    }

    void VulkanBatchRenderer::flush(VkCommandBuffer commandBuffer, uint32_t framebufferWidth, uint32_t framebufferHeight, bool flipY) {
        const VulkanPipeline* boundPipeline = nullptr;
        VkDescriptorSet boundTexture = VK_NULL_HANDLE;
        VkBuffer boundBuffer = VK_NULL_HANDLE;
        VkDeviceSize boundOffset = 0;
        uint32_t boundStencilMode = 0;
        const DrawCommand::State* boundState = nullptr;

        for (const DrawCommand& command : commands) {
            if (command.op == DrawCommand::Op::ClearStencil) {
                VkClearAttachment attachment{};
                attachment.aspectMask = VK_IMAGE_ASPECT_STENCIL_BIT;
                attachment.clearValue.depthStencil = {1.0f, 0};
                VkClearRect rect{};
                rect.rect.extent.width = framebufferWidth;
                rect.rect.extent.height = framebufferHeight;
                rect.layerCount = 1;
                vkCmdClearAttachments(commandBuffer, 1, &attachment, 1, &rect);
                continue;
            }

            const DrawCommand::State& state = command.state;
            VkPipelineLayout layout = state.pipeline->getLayout();

            bool pipelineChanged = state.pipeline != boundPipeline;
            if (pipelineChanged) {
                vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, state.pipeline->getPipeline());
                vkCmdSetLineWidth(commandBuffer, 1.0f);
                boundPipeline = state.pipeline;
            }

            // Viewport and scissor are dynamic in every pipeline, so they survive pipeline changes
            if (boundState == nullptr || std::memcmp(state.viewport, boundState->viewport, sizeof(state.viewport)) != 0) {
                VkViewport viewport{};
                viewport.x = static_cast<float>(state.viewport[0]);
                viewport.width = static_cast<float>(state.viewport[2]);
                if (flipY) {
                    viewport.y = static_cast<float>(static_cast<Int32>(framebufferHeight) - state.viewport[1]);
                    viewport.height = -static_cast<float>(state.viewport[3]);  // VK_KHR_maintenance1
                } else {
                    viewport.y = static_cast<float>(state.viewport[1]);
                    viewport.height = static_cast<float>(state.viewport[3]);
                }
                viewport.maxDepth = 1.0f;
                vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
            }
            if (boundState == nullptr || state.scissorEnabled != boundState->scissorEnabled ||
                (state.scissorEnabled && std::memcmp(state.scissor, boundState->scissor, sizeof(state.scissor)) != 0)) {
                VkRect2D scissor{};
                if (state.scissorEnabled) {
                    scissor = toVulkanRect(state.scissor, framebufferWidth, framebufferHeight, flipY);
                } else {
                    scissor.extent.width = framebufferWidth;
                    scissor.extent.height = framebufferHeight;
                }
                vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
            }

            // Stencil state is dynamic only in the stencil pipelines, so it is set again after a pipeline change
            if (state.stencilMode != 0 && (pipelineChanged || state.stencilMode != boundStencilMode)) {
                setStencilState(commandBuffer, state.stencilMode);
            }
            boundStencilMode = state.stencilMode;

            if (pipelineChanged || state.texture != boundTexture) {
                vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, &state.texture, 0, nullptr);
                boundTexture = state.texture;
            }

            PushConstants constants{};
            std::memcpy(constants.mvp, state.mvp.getStorage(), sizeof(constants.mvp));
            std::memcpy(constants.modulateColor, state.modulateColor, sizeof(constants.modulateColor));
            constants.alphaTest = state.alphaTest ? 1u : 0u;
            constants.time = state.time;
            vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                               sizeof(constants), &constants);

            if (command.vertexBuffer != boundBuffer || command.vertexOffset != boundOffset) {
                vkCmdBindVertexBuffers(commandBuffer, 0, 1, &command.vertexBuffer, &command.vertexOffset);
                boundBuffer = command.vertexBuffer;
                boundOffset = command.vertexOffset;
            }

            vkCmdDraw(commandBuffer, command.vertexCount, 1, 0, 0);
            boundState = &state;
        }

        commands.clear();
    }

}
