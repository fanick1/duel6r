#ifndef DUEL6R_VULKAN_BATCH_RENDERER_H
#define DUEL6R_VULKAN_BATCH_RENDERER_H

#include <vulkan/vulkan.h>
#include <vector>
#include <memory>
#include <unordered_map>
#include "../../Type.h"
#include "../../math/Math.h"
#include "../../math/Matrix.h"
#include "../../Color.h"
#include "VulkanPipeline.h"
#include "VulkanDescriptor.h"
#include "VulkanBuffer.h"

namespace Duel6 {

    // Forward declarations
    class VulkanDevice;
    class VulkanMemoryManager;
    class Console;

    // Draw command for batch rendering
    struct DrawCommand {
        enum Type {
            TRIANGLES,
            TRIANGLE_STRIP,
            LINES,
            LINE_STRIP,
            POINTS
        };

        Type type;
        uint32_t vertexCount;
        uint32_t instanceCount;
        uint32_t firstVertex;
        uint32_t firstInstance;
        
        // Vertex data
        const void* vertices;
        size_t vertexDataSize;
        
        // Index data (optional)
        const uint16_t* indices;
        uint32_t indexCount;
        
        // Transform and material data
        Matrix projectionMatrix;
        Matrix viewMatrix;
        Matrix modelMatrix;
        Color color;
        VkDescriptorSet textureDescriptorSet;

        // Material push constant data (used when textureDescriptorSet != VK_NULL_HANDLE)
        Float32 modulateColor[4];
        uint32_t alphaTest;
        
        // Stencil mode: 0=none, 1=write, 2=test, 3=erase, 4=clear_stencil (special)
        uint32_t stencilMode;

        // Per-draw scissor state
        bool scissorEnabled;
        int32_t scissorX, scissorY, scissorWidth, scissorHeight;

        DrawCommand()
            : type(TRIANGLES)
            , vertexCount(0)
            , instanceCount(1)
            , firstVertex(0)
            , firstInstance(0)
            , vertices(nullptr)
            , vertexDataSize(0)
            , indices(nullptr)
            , indexCount(0)
            , projectionMatrix(Matrix::IDENTITY)
            , viewMatrix(Matrix::IDENTITY)
            , modelMatrix(Matrix::IDENTITY)
            , color(Color::WHITE)
            , textureDescriptorSet(VK_NULL_HANDLE)
            , modulateColor{1.0f, 1.0f, 1.0f, 1.0f}
            , alphaTest(0)
            , stencilMode(0)
            , scissorEnabled(false)
            , scissorX(0), scissorY(0), scissorWidth(0), scissorHeight(0) {}
    };

    // Render batch for grouping similar draw calls
    struct RenderBatch {
        VulkanPipeline* pipeline;
        std::vector<DrawCommand> commands;
        std::vector<std::vector<uint8_t>> vertexDataStorage;  // Persistent storage for vertex data
        VkDescriptorSet descriptorSet;
        bool dirty;
        
        RenderBatch(VulkanPipeline* p = nullptr)
            : pipeline(p)
            , descriptorSet(VK_NULL_HANDLE)
            , dirty(true) {}
    };

    // Batch renderer for efficient Vulkan rendering
    class VulkanBatchRenderer {
    private:
        static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;

        const VulkanDevice& device;
        VulkanMemoryManager& memoryManager;
        Console& console;
        
        // Render batches in submission order (preserves stencil/state ordering)
        std::vector<std::unique_ptr<RenderBatch>> orderedBatches;
        
        // Per-frame vertex/index buffers to avoid GPU/CPU data races.
        // Indexed by currentFrame (0..MAX_FRAMES_IN_FLIGHT-1).
        VulkanVertexBuffer vertexBuffers[MAX_FRAMES_IN_FLIGHT];
        VulkanIndexBuffer indexBuffers[MAX_FRAMES_IN_FLIGHT];
        size_t vertexBufferOffset;
        size_t indexBufferOffset;
        
        // Current frame data
        uint32_t currentFrame;
        uint32_t flushViewportWidth;
        uint32_t flushViewportHeight;
        
        VulkanPipeline* lastPipeline;

        // Deferred draw reordering
        bool deferredMode;
        struct DeferredCommand {
            VulkanPipeline* pipeline;
            DrawCommand command;
            std::vector<uint8_t> vertexData;
        };
        std::vector<DeferredCommand> deferredCommands;

        // Statistics
        struct Statistics {
            uint32_t drawCalls;
            uint32_t triangles;
            uint32_t batches;
            
            void reset() {
                drawCalls = 0;
                triangles = 0;
                batches = 0;
            }
        } stats;

    public:
        VulkanBatchRenderer(const VulkanDevice& device,
                           VulkanMemoryManager& memoryManager,
                           Console& console);
        ~VulkanBatchRenderer();

        // Frame management
        void beginFrame(uint32_t frameIndex);
        void endFrame();
        
        // Command submission
        void addDrawCommand(VulkanPipeline* pipeline, const DrawCommand& command);
        // Flush all pending draw commands into the given command buffer.
        // flipY: when true, uses negative viewport height (VK_KHR_maintenance1) to flip
        // the Y axis so that OpenGL-convention geometry (Y-up, origin bottom-left) renders
        // correctly on screen. Must be true for the final on-screen render pass.
        // For offscreen render targets whose result is later sampled as a texture, flipY
        // must be false — Vulkan textures have V=0 at the top, so the offscreen image is
        // already in the correct orientation for texture sampling. If flipY were true, the
        // offscreen content would be upside-down when applied as a fullscreen quad, because
        // the main render pass applies its own Y flip on top.
        void flush(VkCommandBuffer commandBuffer, VkRenderPass renderPass,
                   uint32_t viewportWidth, uint32_t viewportHeight,
                   bool flipY = true);
        
        // Buffer management
        void reserveVertexData(size_t size);
        void reserveIndexData(size_t size);
        
        // Statistics
        const Statistics& getStatistics() const { return stats; }
        void resetStatistics() { stats.reset(); }
        
        // Deferred draw reordering
        void setDeferredMode(bool enable);

        // Utility methods
        void clear();
        bool isEmpty() const;
        
    private:
        RenderBatch* getBatch(VulkanPipeline* pipeline);
        bool canMergeCommands(const DrawCommand& a, const DrawCommand& b) const;
        void flushDeferred();
        void updateUniformBuffer(RenderBatch& batch);
        void recordDrawCommands(VkCommandBuffer commandBuffer, const RenderBatch& batch);
        VkPrimitiveTopology getVulkanTopology(DrawCommand::Type type);
        
        // Buffer allocation helpers
        void ensureVertexBufferCapacity(size_t requiredSize);
        void ensureIndexBufferCapacity(size_t requiredSize);
    };

}

#endif // DUEL6R_VULKAN_BATCH_RENDERER_H
