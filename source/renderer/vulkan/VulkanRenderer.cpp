#include "VulkanRenderer.h"
#include "../../Format.h"

namespace Duel6 {
    VulkanRenderer::VulkanRenderer(SDL_Window* window, Console& console)
        : window(window)
        , console(&console)
        , initialized(false)
        , framebufferResized(false)
        , context(nullptr)
        , device(nullptr)
        , surface(nullptr)
        , swapchain(nullptr)
        , memoryManager(nullptr)
        , projectionMatrix(Matrix::IDENTITY)
        , viewMatrix(Matrix::IDENTITY)
        , modelMatrix(Matrix::IDENTITY) {
        
        initialize();
    }

    VulkanRenderer::~VulkanRenderer() {
        cleanup();
    }

    void VulkanRenderer::initialize() {
        if (initialized) {
            return;
        }

        try {
            // Create core Vulkan components
            context = std::make_unique<VulkanContext>(*console);
            device = std::make_unique<VulkanDevice>(*console);
            surface = std::make_unique<VulkanSurface>(*console);
            swapchain = std::make_unique<VulkanSwapchain>(*console);
            memoryManager = std::make_unique<VulkanMemoryManager>(*console);

            // Initialize in dependency order
            context->initialize(window);
            surface->initialize(*context, window);
            device->initialize(*context, surface->getSurface());
            swapchain->initialize(*device, *surface);
            memoryManager->initialize(*device, context->getInstance());

            // Initialize rendering components
            descriptorManager = std::make_unique<VulkanDescriptorManager>(*console);
            descriptorManager->initialize();
            batchRenderer = std::make_unique<VulkanBatchRenderer>(*device, *memoryManager, *console);
            
            // Create render pass with default configuration
            RenderPassConfig renderPassConfig;
            renderPassConfig.colorFormat = swapchain->getImageFormat();
            renderPassConfig.clearColorValue = {{0.0f, 0.0f, 0.2f, 1.0f}}; // Dark blue
            renderPass = std::make_unique<VulkanRenderPass>(*device, renderPassConfig);
            
            commandBufferManager = std::make_unique<VulkanCommandBufferManager>(*device, 2);
            
            // Initialize pipeline cache and create basic pipelines
            pipelineCache = std::make_unique<VulkanPipelineCache>(*device);
            colorPipeline = nullptr;
            texturePipeline = nullptr;
            createPipelines();
            
            currentFrame = 0;

            initialized = true;
            console->printLine("...Vulkan renderer initialized successfully");
        } catch (const VulkanException& e) {
            console->printLine(Format("Failed to initialize Vulkan renderer: {0}") << e.getMessage());
            cleanup();
            throw;
        }
    }

    void VulkanRenderer::cleanup() {
        if (!initialized) {
            return;
        }

        if (device) {
            device->waitIdle();
        }

        // Cleanup rendering components
        colorPipeline = nullptr;
        texturePipeline = nullptr;
        pipelineCache.reset();
        commandBufferManager.reset();
        renderPass.reset();
        batchRenderer.reset();
        if (descriptorManager) {
            descriptorManager->cleanup(*device);
            descriptorManager.reset();
        }

        if (memoryManager) {
            memoryManager->cleanup(*device);
            memoryManager.reset();
        }

        if (swapchain) {
            swapchain->cleanup(*device);
            swapchain.reset();
        }

        if (device) {
            device->cleanup();
            device.reset();
        }

        if (surface) {
            surface->cleanup(*context);
            surface.reset();
        }

        if (context) {
            context->cleanup();
            context.reset();
        }

        initialized = false;
        console->printLine("...Vulkan renderer cleaned up");
    }

    void VulkanRenderer::recreateSwapchain() {
        if (!initialized) {
            return;
        }

        int width = 0, height = 0;
        SDL_GetWindowSize(window, &width, &height);
        while (width == 0 || height == 0) {
            SDL_GetWindowSize(window, &width, &height);
            SDL_WaitEvent(nullptr);
        }

        device->waitIdle();
        swapchain->recreate(*device, *surface);
        framebufferResized = false;
    }

    void VulkanRenderer::onFramebufferResize() {
        framebufferResized = true;
    }

    Renderer::Info VulkanRenderer::getInfo() {
        if (!initialized) {
            return {"Unknown", "Vulkan Renderer (Not Initialized)", "Unknown"};
        }

        VkPhysicalDeviceProperties properties = device->getDeviceProperties();
        
        Info info;
        info.vendor = "Vulkan";
        info.renderer = std::string(properties.deviceName);
        info.version = Format("Vulkan {0}.{1}.{2}")
            << VK_VERSION_MAJOR(properties.apiVersion)
            << VK_VERSION_MINOR(properties.apiVersion)
            << VK_VERSION_PATCH(properties.apiVersion);
        
        return info;
    }

    Renderer::Extensions VulkanRenderer::getExtensions() {
        Extensions extensions;
        // TODO: Implement when needed
        return extensions;
    }

    Texture VulkanRenderer::createTexture(const Image &image, TextureFilter filtering, bool clamp) {
        // TODO: Implement texture creation
        console->printLine("Warning: VulkanRenderer::createTexture not yet implemented");
        return INVALID_TEXTURE_ID;
    }

    void VulkanRenderer::freeTexture(Texture textureId) {
        // TODO: Implement texture freeing
        console->printLine("Warning: VulkanRenderer::freeTexture not yet implemented");
    }

    Image VulkanRenderer::makeScreenshot() {
        // TODO: Implement screenshot
        console->printLine("Warning: VulkanRenderer::makeScreenshot not yet implemented");
        return Image();
    }

    void VulkanRenderer::setViewport(Int32 x, Int32 y, Int32 width, Int32 height) {
        if (!initialized) {
            return;
        }
        
        // Store viewport parameters - they'll be used in command recording
        // For now, just log the viewport change
        console->printLine(Format("Viewport set to {0}x{1} at ({2},{3})") << width << height << x << y);
        
        // TODO: Update viewport state for rendering
    }

    void VulkanRenderer::enableWireframe(bool enable) {
        // TODO: Implement wireframe mode
        console->printLine("Warning: VulkanRenderer::enableWireframe not yet implemented");
    }

    void VulkanRenderer::enableDepthTest(bool enable) {
        // TODO: Implement depth testing
        console->printLine("Warning: VulkanRenderer::enableDepthTest not yet implemented");
    }

    void VulkanRenderer::enableDepthWrite(bool enable) {
        // TODO: Implement depth writing
        console->printLine("Warning: VulkanRenderer::enableDepthWrite not yet implemented");
    }

    void VulkanRenderer::setBlendFunc(BlendFunc func) {
        // TODO: Implement blend function
        console->printLine("Warning: VulkanRenderer::setBlendFunc not yet implemented");
    }

    void VulkanRenderer::setGlobalTime(Float32 time) {
        // TODO: Implement global time for shaders
        console->printLine("Warning: VulkanRenderer::setGlobalTime not yet implemented");
    }

    void VulkanRenderer::clearBuffers() {
        if (!initialized) {
            return;
        }

        // Handle swapchain recreation if needed
        if (framebufferResized || !swapchain->acquireNextImage(*device)) {
            recreateSwapchain();
            return;
        }

        // Begin frame in batch renderer
        batchRenderer->beginFrame(currentFrame);
        
        // Begin command recording
        commandBufferManager->beginFrame();
        VkCommandBuffer commandBuffer = commandBufferManager->getCurrentCommandBuffer();
        
        // Begin render pass with clear
        uint32_t framebufferIndex = swapchain->getCurrentImageIndex();
        renderPass->begin(commandBuffer, framebufferIndex);
        
        // Flush any pending draw commands
        batchRenderer->flush(commandBuffer, renderPass->getHandle());
        
        // End render pass
        renderPass->end(commandBuffer);
        
        // End command recording and submit
        commandBufferManager->endFrame();
        batchRenderer->endFrame();
        
        swapchain->present(*device);
        currentFrame = (currentFrame + 1) % 2; // Assuming MAX_FRAMES_IN_FLIGHT = 2
        commandBufferManager->nextFrame();
    }

    // Geometry rendering - implementations
    void VulkanRenderer::triangle(const Vector &p1, const Vector &p2, const Vector &p3, const Color &color) {
        if (!initialized || !batchRenderer) {
            return;
        }

        // Create vertex data for colored triangle using proper VertexFormats
        VertexFormats::ColorVertex vertices[3] = {
            {p1, {color.getRed() / 255.0f, color.getGreen() / 255.0f, color.getBlue() / 255.0f, color.getAlpha() / 255.0f}},
            {p2, {color.getRed() / 255.0f, color.getGreen() / 255.0f, color.getBlue() / 255.0f, color.getAlpha() / 255.0f}},
            {p3, {color.getRed() / 255.0f, color.getGreen() / 255.0f, color.getBlue() / 255.0f, color.getAlpha() / 255.0f}}
        };

        // Create draw command
        DrawCommand cmd;
        cmd.type = DrawCommand::TRIANGLES;
        cmd.vertexCount = 3;
        cmd.vertices = vertices;
        cmd.vertexDataSize = sizeof(vertices);
        cmd.projectionMatrix = getProjectionMatrix();
        cmd.viewMatrix = getViewMatrix();
        cmd.modelMatrix = getModelMatrix();
        cmd.color = color;

        // Get color pipeline and add to batch renderer
        VulkanPipeline* pipeline = getColorPipeline();
        if (pipeline) {
            batchRenderer->addDrawCommand(pipeline, cmd);
        } else {
            console->printLine("Warning: Color pipeline not available for triangle rendering");
        }
    }

    void VulkanRenderer::triangle(const Vector &p1, const Vector &t1,
                                  const Vector &p2, const Vector &t2,
                                  const Vector &p3, const Vector &t3,
                                  const Material &material) {
        // TODO: Implement textured triangle rendering
    }

    void VulkanRenderer::quad(const Vector &p1, const Vector &p2, const Vector &p3, const Vector &p4, const Color &color) {
        // TODO: Implement quad rendering
    }

    void VulkanRenderer::quad(const Vector &p1, const Vector &t1,
                              const Vector &p2, const Vector &t2,
                              const Vector &p3, const Vector &t3,
                              const Vector &p4, const Vector &t4,
                              const Material &material) {
        // TODO: Implement textured quad rendering
    }

    void VulkanRenderer::point(const Vector &position, Float32 size, const Color &color) {
        // TODO: Implement point rendering
    }

    void VulkanRenderer::line(const Vector &from, const Vector &to, Float32 width, const Color &color) {
        // TODO: Implement line rendering
    }

    void VulkanRenderer::lines(const Vector positions[4], Float32 width, const Color &color) {
        // TODO: Implement lines rendering
    }

    void VulkanRenderer::lines(const std::vector<Vector> &positions, const std::vector<int>& indices, 
                               const int restartIndex, Float32 width, const Color &color) {
        // TODO: Implement indexed lines rendering
    }

    void VulkanRenderer::stencil(StencilMode mode) const {
        // TODO: Implement stencil operations
    }

    bool VulkanRenderer::setScissorTest(bool value) const {
        // TODO: Implement scissor test
        return false;
    }

    void VulkanRenderer::scissorTest(Int32 x, Int32 y, Int32 width, Int32 height) const {
        // TODO: Implement scissor testing
    }

    std::unique_ptr<RendererBuffer> VulkanRenderer::makeBuffer(const FaceList &faceList) {
        // TODO: Implement buffer creation
        console->printLine("Warning: VulkanRenderer::makeBuffer not yet implemented");
        return nullptr;
    }

    std::unique_ptr<RendererTarget> VulkanRenderer::makeTarget(ScreenParameters screenParameters) {
        // TODO: Implement render target creation
        console->printLine("Warning: VulkanRenderer::makeTarget not yet implemented");
        return nullptr;
    }

    void VulkanRenderer::createPipelines() {
        if (!initialized || !pipelineCache || !renderPass || !descriptorManager) {
            return;
        }

        try {
            // Get proper uniform buffer layout from descriptor manager
            VkDescriptorSetLayout uniformLayout = descriptorManager->getUniformBufferLayout(*device);
            
            // Create color pipeline for simple colored geometry using preset
            auto colorPipelinePtr = PipelinePresets::createColorPipeline(
                *device, 
                renderPass->getHandle(),
                uniformLayout
            );

            if (colorPipelinePtr) {
                // Store in cache
                size_t hash = std::hash<std::string>{}("color_pipeline");
                colorPipeline = pipelineCache->getOrCreatePipeline(hash, 
                    [&]() { return std::move(colorPipelinePtr); });
                console->printLine("...Created color pipeline");
            }
        } catch (const std::exception& e) {
            console->printLine(Format("Failed to create color pipeline: {0}") << e.what());
        }
    }

    VulkanPipeline* VulkanRenderer::getColorPipeline() {
        if (!colorPipeline) {
            createPipelines();
        }
        return colorPipeline;
    }

    VulkanPipeline* VulkanRenderer::getTexturePipeline() {
        if (!texturePipeline) {
            createPipelines();
        }
        return texturePipeline;
    }

    void VulkanRenderer::setProjectionMatrix(const Matrix &m) {
        projectionMatrix = m;
    }

    Matrix VulkanRenderer::getProjectionMatrix() const {
        return projectionMatrix;
    }

    void VulkanRenderer::setViewMatrix(const Matrix &m) {
        viewMatrix = m;
    }

    Matrix VulkanRenderer::getViewMatrix() const {
        return viewMatrix;
    }

    void VulkanRenderer::setModelMatrix(const Matrix &m) {
        modelMatrix = m;
    }

    Matrix VulkanRenderer::getModelMatrix() const {
        return modelMatrix;
    }
}