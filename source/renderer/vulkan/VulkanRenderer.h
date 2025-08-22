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
#include "VulkanDescriptor.h"
#include "VulkanRenderPass.h"
#include <SDL2/SDL.h>
#include <memory>

namespace Duel6 {
    class VulkanRenderer : public RendererBase {
    private:
        SDL_Window* window;
        Console* console;
        bool initialized;
        bool framebufferResized;

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
        
        // Current frame state
        uint32_t currentFrame;

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
        void onFramebufferResize();

    private:
        void initialize();
        void cleanup();
        void recreateSwapchain();
    };
}

#endif