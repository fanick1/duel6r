#ifndef DUEL6_RENDERER_VULKANRENDERER_H
#define DUEL6_RENDERER_VULKANRENDERER_H

#include "../RendererBase.h"
#include "VulkanTypes.h"
#include "VulkanContext.h"
#include "VulkanDevice.h"
#include "VulkanSurface.h"
#include "VulkanSwapchain.h"
#include "VulkanMemory.h"
#include "VulkanBatchRenderer.h"
#include "VulkanPipeline.h"
#include "VulkanDescriptor.h"
#include "VulkanRenderPass.h"
#include "VulkanRendererTarget.h"
#include "VulkanResourceManager.h"
#include "VulkanSampler.h"
#include <SDL2/SDL.h>
#include <memory>

namespace Duel6 {
    class VulkanRenderer : public RendererBase {
    private:
        SDL_Window* window;
        Console* console;
        bool initialized;

        // Core Vulkan components
        std::unique_ptr<VulkanContext> context;
        std::unique_ptr<VulkanDevice> device;
        std::unique_ptr<VulkanSurface> surface;
        std::unique_ptr<VulkanSwapchain> swapchain;
        std::unique_ptr<VulkanMemoryManager> memoryManager;
        
        // Rendering components
        std::unique_ptr<VulkanDescriptorManager> descriptorManager;
        std::unique_ptr<VulkanBatchRenderer> batchRenderer;
        std::unique_ptr<VulkanRenderPass> renderPass;
        std::unique_ptr<VulkanCommandBufferManager> commandBufferManager;
        
        // Pipeline cache
        std::unique_ptr<VulkanPipelineCache> pipelineCache;

        // Resource management (textures)
        std::unique_ptr<VulkanSamplerManager> samplerManager;
        std::unique_ptr<VulkanResourceManager> resourceManager;
        
        // Current frame state
        uint32_t currentFrame;
        bool inFrame;  // True between clearBuffers() and swapBuffers()

        // Copy of the last rendered frame for makeScreenshot(). Swapchain images may only be
        // touched while acquired, so each frame is copied here before it is presented.
        VkImage captureImage = VK_NULL_HANDLE;
        VkDeviceMemory captureMemory = VK_NULL_HANDLE;
        VkExtent2D captureExtent{};
        bool captureValid = false;
        
        // Scissor state
        mutable bool scissorEnabled;

        // Viewport state (stored, applied at flush)
        mutable Int32 viewportX, viewportY, viewportWidth, viewportHeight;

        // Scissor extents
        mutable Int32 scissorX, scissorY, scissorWidth, scissorHeight;

        // Global time for shader effects
        Float32 globalTime;

        // Stencil state
        mutable Renderer::StencilMode currentStencilMode;

        // Matrix state
        Matrix projectionMatrix;
        Matrix viewMatrix;  
        Matrix modelMatrix;

        // Render state (tracked for pipeline selection)
        bool depthTestEnabled;
        bool depthWriteEnabled;
        BlendFunc currentBlendFunc;
        bool wireframeEnabled;
        VkDescriptorSetLayout textureDescriptorSetLayout; // Layout for texture-only descriptor sets
        VkDescriptorSet whiteTexture = VK_NULL_HANDLE;    // 1x1 white, sampled by untextured draws
        bool offscreen = false;                           // drawing into an (upside down) offscreen target

    public:
        explicit VulkanRenderer(SDL_Window* window, Console& console);
        ~VulkanRenderer();

        // Non-copyable
        VulkanRenderer(const VulkanRenderer&) = delete;
        VulkanRenderer& operator=(const VulkanRenderer&) = delete;

        // Renderer interface implementation
        Info getInfo() override;
        Extensions getExtensions() override;

        Texture createTexture(const Image &image, TextureFilter filtering, bool clamp) override;
        void freeTexture(Texture textureId) override;
        Image makeScreenshot() override;

        void setViewport(Int32 x, Int32 y, Int32 width, Int32 height) override;
        void enableWireframe(bool enable) override;
        void enableDepthTest(bool enable) override;
        void enableDepthWrite(bool enable) override;
        void setBlendFunc(BlendFunc func) override;
        void setGlobalTime(Float32 time) override;
        void clearBuffers() override;
        void swapBuffers();
        
        // Matrix methods
        void setProjectionMatrix(const Matrix &m) override;
        void setViewMatrix(const Matrix &m) override;
        Matrix getViewMatrix() const override;
        void setModelMatrix(const Matrix &m) override;

        // Geometry rendering
        void triangle(const Vector &p1, const Vector &p2, const Vector &p3, const Color &color) override;
        void triangle(const Vector &p1, const Vector &t1,
                      const Vector &p2, const Vector &t2,
                      const Vector &p3, const Vector &t3,
                      const Material &material) override;

        void quad(const Vector &p1, const Vector &p2, const Vector &p3, const Vector &p4, const Color &color) override;
        void quad(const Vector &p1, const Vector &t1,
                  const Vector &p2, const Vector &t2,
                  const Vector &p3, const Vector &t3,
                  const Vector &p4, const Vector &t4,
                  const Material &material) override;

        void point(const Vector &position, Float32 size, const Color &color) override;
        void line(const Vector &from, const Vector &to, Float32 width, const Color &color) override;
        void lines(const Vector positions[4], Float32 width, const Color &color) override;
        void lines(const std::vector<Vector> &positions, const std::vector<int>& indices, const int restartIndex, Float32 width, const Color &color) override;

        // Stencil and scissor operations
        void stencil(StencilMode mode) const override;
        bool setScissorTest(bool value) const override;
        void scissorTest(Int32 x, Int32 y, Int32 width, Int32 height) const override;


        // Buffer and target creation
        std::unique_ptr<RendererBuffer> makeBuffer(const FaceList &faceList) override;
        std::unique_ptr<RendererTarget> makeTarget(ScreenParameters screenParameters) override;

        // Window resize handling

    private:
        void initialize();
        void cleanup();
        void recreateSwapchain();
        void ensureCaptureImage(VkExtent2D extent);
        void destroyCaptureImage();
        void recordFrameCapture(VkCommandBuffer commandBuffer);
        VulkanPipeline* getPipeline(VkPrimitiveTopology topology);

    public:
        // Called by VulkanRendererBuffer to render pre-built material vertices
        // Draw state (stencil, viewport, scissor, transform) of the next draw
        DrawCommand::State makeDrawState(VkPrimitiveTopology topology) const;
        // Submits vertices with the current render state; untextured when material is null
        void submitVertices(VkPrimitiveTopology topology, const VulkanVertex* vertices, uint32_t vertexCount,
                            const Material* material);
        // Pipeline for an explicit render state (lazily created and cached)
        VulkanPipeline* getPipeline(VkPrimitiveTopology topology, bool depthTest, bool depthWrite,
                                    BlendFunc blendFunc, bool wireframe, uint32_t stencilMode);

        // Accessors for VulkanRendererTarget offscreen rendering
        VulkanDevice& getDevice() { return *device; }
        const VulkanDevice& getDevice() const { return *device; }
        VulkanBatchRenderer& getBatchRenderer() { return *batchRenderer; }
        VulkanDescriptorManager& getDescriptorManager() { return *descriptorManager; }
        VulkanSamplerManager& getSamplerManager() { return *samplerManager; }
        VulkanRenderPass& getMainRenderPass() { return *renderPass; }
        VulkanPipelineCache& getPipelineCache() { return *pipelineCache; }
        VkDescriptorSetLayout getTextureDescriptorSetLayout() const { return textureDescriptorSetLayout; }
        bool isInFrame() const { return inFrame; }
        void setInFrame(bool value) { inFrame = value; }
        void setOffscreen(bool value) { offscreen = value; }
        uint32_t getCurrentFrameIndex() const { return currentFrame; }
        bool isInitialized() const { return initialized; }

        // Flush and manage render pass for offscreen rendering
    };
}

#endif