#ifndef DUEL6R_VULKAN_BATCH_RENDERER_H
#define DUEL6R_VULKAN_BATCH_RENDERER_H

#include <vulkan/vulkan.h>
#include <vector>
#include "../../Type.h"
#include "../../math/Matrix.h"
#include "VulkanMemory.h"
#include "VulkanTypes.h"

namespace Duel6 {

    class VulkanDevice;
    class VulkanPipeline;
    class Console;
    class Color;

    // One draw as recorded by the renderer's immediate-mode calls and replayed by flush().
    // Vertex data is copied into the frame's vertex buffer the moment the draw is added.
    struct DrawCommand {
        enum class Op {
            Draw,
            ClearStencil
        };

        // Everything that must be equal for two draws to be issued as one
        struct State {
            VulkanPipeline* pipeline = nullptr;
            VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            VkDescriptorSet texture = VK_NULL_HANDLE;  // untextured draws use the renderer's white texture
            Float32 modulateColor[4] = {1.0f, 1.0f, 1.0f, 1.0f};
            bool alphaTest = false;
            Float32 time = 0.0f;                       // drives the water wave animation
            uint32_t stencilMode = 0;                  // 0 none, 1 write, 2 test, 3 erase
            Int32 viewport[4] = {0, 0, 0, 0};          // x, y, width, height in OpenGL convention (y up)
            bool scissorEnabled = false;
            Int32 scissor[4] = {0, 0, 0, 0};           // same convention as the viewport
            Matrix mvp = Matrix::IDENTITY;

            void setModulateColor(const Color& color);
            bool operator==(const State& other) const;
        };

        Op op = Op::Draw;
        State state;
        VkBuffer vertexBuffer = VK_NULL_HANDLE;
        VkDeviceSize vertexOffset = 0;
        uint32_t vertexCount = 0;
    };

    // Collects the draws of one frame and records them into a command buffer in order.
    //
    // Vertices live in one host-visible, permanently mapped buffer per frame in flight, so a
    // draw costs one copy. Consecutive draws with the same state are merged into one vkCmdDraw.
    class VulkanBatchRenderer {
    public:
        static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;

        VulkanBatchRenderer(const VulkanDevice& device, VulkanMemoryManager& memoryManager, Console& console);
        ~VulkanBatchRenderer();

        VulkanBatchRenderer(const VulkanBatchRenderer&) = delete;
        VulkanBatchRenderer& operator=(const VulkanBatchRenderer&) = delete;

        // Starts reusing the given frame slot. The GPU must have finished the frame that last used it.
        void beginFrame(uint32_t frameIndex);

        void draw(const DrawCommand::State& state, const VulkanVertex* vertices, uint32_t vertexCount);
        void clearStencil();

        // Records all pending draws into the command buffer, inside a render pass whose framebuffer
        // has the given size, and forgets them. With flipY the viewport is inverted so that
        // OpenGL-convention geometry (y up) lands right side up; without it the image is stored
        // upside down, like an OpenGL framebuffer object.
        void flush(VkCommandBuffer commandBuffer, uint32_t framebufferWidth, uint32_t framebufferHeight, bool flipY);

        // Drops pending draws without recording them (e.g. when their pipelines are being destroyed)
        void discard();

    private:
        // Vertex storage of one frame slot. Normally a single buffer; when a frame needs more, further
        // buffers are chained and the slot is consolidated into one large buffer at its next beginFrame.
        struct FrameVertices {
            std::vector<VulkanBuffer> buffers;
            size_t current = 0;
            VkDeviceSize offset = 0;
            VkDeviceSize used = 0;
        };

        static constexpr VkDeviceSize INITIAL_BUFFER_SIZE = 2 * 1024 * 1024;

        const VulkanDevice& device;
        VulkanMemoryManager& memoryManager;
        Console& console;

        FrameVertices frames[MAX_FRAMES_IN_FLIGHT];
        uint32_t currentFrame = 0;
        std::vector<DrawCommand> commands;

        VulkanBuffer createVertexBuffer(VkDeviceSize size);
        void* allocateVertices(VkDeviceSize size, VkBuffer& buffer, VkDeviceSize& offset);
        bool canMerge(const DrawCommand& last, const DrawCommand& next) const;
    };

}

#endif // DUEL6R_VULKAN_BATCH_RENDERER_H
