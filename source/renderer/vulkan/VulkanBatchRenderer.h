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
            , textureDescriptorSet(VK_NULL_HANDLE) {}
    };

    // Render batch for grouping similar draw calls
    struct RenderBatch {
        VulkanPipeline* pipeline;
        std::vector<DrawCommand> commands;
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
        const VulkanDevice& device;
        VulkanMemoryManager& memoryManager;
        Console& console;
        
        // Render batches organized by pipeline
        std::unordered_map<VulkanPipeline*, std::unique_ptr<RenderBatch>> batches;
        
        // Vertex buffer management
        VulkanVertexBuffer vertexBuffer;
        VulkanIndexBuffer indexBuffer;
        size_t vertexBufferOffset;
        size_t indexBufferOffset;
        
        // Current frame data
        uint32_t currentFrame;
        static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;
        
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
        void flush(VkCommandBuffer commandBuffer, VkRenderPass renderPass);
        
        // Buffer management
        void reserveVertexData(size_t size);
        void reserveIndexData(size_t size);
        
        // Statistics
        const Statistics& getStatistics() const { return stats; }
        void resetStatistics() { stats.reset(); }
        
        // Utility methods
        void clear();
        bool isEmpty() const;
        
    private:
        RenderBatch* getBatch(VulkanPipeline* pipeline);
        void updateUniformBuffer(RenderBatch& batch);
        void recordDrawCommands(VkCommandBuffer commandBuffer, const RenderBatch& batch);
        VkPrimitiveTopology getVulkanTopology(DrawCommand::Type type);
        
        // Buffer allocation helpers
        void ensureVertexBufferCapacity(size_t requiredSize);
        void ensureIndexBufferCapacity(size_t requiredSize);
    };

}

#endif // DUEL6R_VULKAN_BATCH_RENDERER_H
