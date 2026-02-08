#include "VulkanRenderer.h"
#include "../../Format.h"
#include "../../FaceList.h"
#include "../../Vertex.h"

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
        , currentFrame(0)
        , inFrame(false)
        , scissorEnabled(false)
        , viewportX(0)
        , viewportY(0)
        , viewportWidth(0)
        , viewportHeight(0)
        , viewportDirty(false)
        , scissorX(0)
        , scissorY(0)
        , scissorWidth(0)
        , scissorHeight(0)
        , globalTime(0.0f)
        , currentStencilMode(Renderer::StencilMode::DISABLE)
        , projectionMatrix(Matrix::IDENTITY)
        , viewMatrix(Matrix::IDENTITY)
        , modelMatrix(Matrix::IDENTITY)
        , depthTestEnabled(false)
        , depthWriteEnabled(true)
        , currentBlendFunc(BlendFunc::None)
        , wireframeEnabled(false)
        , textureDescriptorSetLayout(VK_NULL_HANDLE) {
        
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

            // Initialize resource management
            samplerManager = std::make_unique<VulkanSamplerManager>(*console);
            resourceManager = std::make_unique<VulkanResourceManager>(*console);
            resourceManager->initialize(memoryManager.get(), samplerManager.get(), descriptorManager.get());

            // Cache the texture descriptor set layout for pipeline creation
            textureDescriptorSetLayout = descriptorManager->getTextureLayout(*device);

            batchRenderer = std::make_unique<VulkanBatchRenderer>(*device, *memoryManager, *console);
            
            // Create render pass with default configuration
            RenderPassConfig renderPassConfig;
            renderPassConfig.colorFormat = swapchain->getImageFormat();
            renderPassConfig.clearColorValue = {{0.0f, 0.0f, 0.2f, 1.0f}}; // Dark blue
            renderPass = std::make_unique<VulkanRenderPass>(*device, renderPassConfig);
            
            // Create framebuffers for the render pass
            renderPass->createFramebuffers(*swapchain);
            
            commandBufferManager = std::make_unique<VulkanCommandBufferManager>(*device, 2);
            
            // Initialize pipeline cache and create basic pipelines
            pipelineCache = std::make_unique<VulkanPipelineCache>(*device);
            colorPipeline = nullptr;
            linePipeline = nullptr;
            lineStripPipeline = nullptr;
            pointPipeline = nullptr;
            texturePipeline = nullptr;
            createPipelines();
            
            currentFrame = 0;
            inFrame = false;

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
        linePipeline = nullptr;
        lineStripPipeline = nullptr;
        pointPipeline = nullptr;
        texturePipeline = nullptr;
        textureDescriptorSetLayout = VK_NULL_HANDLE;
        pipelineCache.reset();
        commandBufferManager.reset();
        renderPass.reset();
        batchRenderer.reset();

        if (resourceManager && device) {
            resourceManager->cleanup(*device);
            resourceManager.reset();
        }
        if (samplerManager && device) {
            samplerManager->cleanup(*device);
        }
        samplerManager.reset();

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
        
        // Clear batches BEFORE destroying pipelines to avoid dangling pipeline pointers
        if (batchRenderer) {
            batchRenderer->clear();
        }
        
        swapchain->recreate(*device, *surface);
        
        // Recreate render pass if swapchain format changed
        VkFormat newFormat = swapchain->getImageFormat();
        if (renderPass && renderPass->getConfig().colorFormat != newFormat) {
            // Format changed — need to recreate the entire render pass
            RenderPassConfig newConfig = renderPass->getConfig();
            newConfig.colorFormat = newFormat;
            renderPass = std::make_unique<VulkanRenderPass>(*device, newConfig);
            renderPass->createFramebuffers(*swapchain);
        } else if (renderPass) {
            // Same format — just recreate framebuffers
            renderPass->recreateFramebuffers(*swapchain);
        }
        
        // Recreate pipelines (they reference the render pass)
        if (pipelineCache) {
            pipelineCache = std::make_unique<VulkanPipelineCache>(*device);
            colorPipeline = nullptr;
            linePipeline = nullptr;
            lineStripPipeline = nullptr;
            pointPipeline = nullptr;
            texturePipeline = nullptr;
            createPipelines();
        }
        
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
        if (!initialized || !resourceManager) {
            console->printLine("Warning: VulkanRenderer::createTexture called but renderer not ready");
            return INVALID_TEXTURE_ID;
        }

        try {
            return resourceManager->createTexture(*device, image, filtering, clamp);
        } catch (const std::exception& e) {
            console->printLine(Format("Failed to create texture: {0}") << e.what());
            return INVALID_TEXTURE_ID;
        }
    }

    void VulkanRenderer::freeTexture(Texture textureId) {
        if (!initialized || !resourceManager) {
            return;
        }

        if (textureId != INVALID_TEXTURE_ID) {
            resourceManager->deleteTexture(textureId);
        }
    }

    Image VulkanRenderer::makeScreenshot() {
        if (!initialized || !device || !swapchain || !memoryManager) {
            return Image();
        }

        device->waitIdle();

        VkExtent2D extent = swapchain->getExtent();
        uint32_t width = extent.width;
        uint32_t height = extent.height;
        VkDeviceSize imageSize = width * height * 4; // RGBA 8-bit

        // Get the last presented swapchain image
        uint32_t imageIndex = swapchain->getCurrentImageIndex();
        VkImage srcImage = swapchain->getImage(imageIndex);

        // Create a host-visible staging buffer
        VulkanBuffer stagingBuffer = memoryManager->createStagingBuffer(*device, imageSize);

        // Record a single-time command buffer
        VkCommandBuffer cmdBuf = device->beginSingleTimeCommands();

        // Transition swapchain image from PRESENT_SRC to TRANSFER_SRC
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = srcImage;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier(cmdBuf,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
            0, 0, nullptr, 0, nullptr, 1, &barrier);

        // Copy image to staging buffer
        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;   // tightly packed
        region.bufferImageHeight = 0; // tightly packed
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = {0, 0, 0};
        region.imageExtent = {width, height, 1};

        vkCmdCopyImageToBuffer(cmdBuf, srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                               stagingBuffer.buffer, 1, &region);

        // Transition swapchain image back to PRESENT_SRC
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;

        vkCmdPipelineBarrier(cmdBuf,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
            0, 0, nullptr, 0, nullptr, 1, &barrier);

        device->endSingleTimeCommands(cmdBuf);

        // Read pixels from staging buffer
        void* data = memoryManager->mapBuffer(*device, stagingBuffer);
        if (!data) {
            memoryManager->destroyBuffer(*device, stagingBuffer);
            return Image();
        }

        Image image(width, height);
        const uint8_t* src = static_cast<const uint8_t*>(data);
        bool isBGRA = (swapchain->getImageFormat() == VK_FORMAT_B8G8R8A8_UNORM ||
                       swapchain->getImageFormat() == VK_FORMAT_B8G8R8A8_SRGB);

        for (uint32_t y = 0; y < height; y++) {
            for (uint32_t x = 0; x < width; x++) {
                size_t idx = (y * width + x) * 4;
                uint8_t r = isBGRA ? src[idx + 2] : src[idx + 0];
                uint8_t g = src[idx + 1];
                uint8_t b = isBGRA ? src[idx + 0] : src[idx + 2];
                uint8_t a = src[idx + 3];
                image.at(y * width + x) = Color(r, g, b, a);
            }
        }

        memoryManager->unmapBuffer(*device, stagingBuffer);
        memoryManager->destroyBuffer(*device, stagingBuffer);

        return image;
    }

    void VulkanRenderer::setViewport(Int32 x, Int32 y, Int32 width, Int32 height) {
        if (!initialized) {
            return;
        }
        
        viewportX = x;
        viewportY = y;
        viewportWidth = width;
        viewportHeight = height;
        viewportDirty = true;
    }

    void VulkanRenderer::enableWireframe(bool enable) {
        wireframeEnabled = enable;
    }

    void VulkanRenderer::enableDepthTest(bool enable) {
        depthTestEnabled = enable;
    }

    void VulkanRenderer::enableDepthWrite(bool enable) {
        depthWriteEnabled = enable;
    }

    void VulkanRenderer::setBlendFunc(BlendFunc func) {
        currentBlendFunc = func;
    }

    void VulkanRenderer::setGlobalTime(Float32 time) {
        globalTime = time;
        // Time is stored and will be passed to shaders when UBO-based material pipeline is implemented
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
        
        // Mark that we're now inside a frame
        inFrame = true;
        
        // NOTE: Don't flush or end render pass here!
        // The geometry will be added via quad(), triangle(), etc. calls
        // and then flushed in swapBuffers()
    }

    void VulkanRenderer::swapBuffers() {
        if (!initialized || !inFrame) {
            return;
        }
        
        // Mark that we're ending the frame
        inFrame = false;
        
        VkCommandBuffer commandBuffer = commandBufferManager->getCurrentCommandBuffer();
        
        // Get swapchain extent for viewport
        VkExtent2D extent = swapchain->getExtent();
        
        // Use explicit viewport if setViewport was called, otherwise use swapchain extent
        uint32_t vpWidth = viewportDirty ? static_cast<uint32_t>(viewportWidth) : extent.width;
        uint32_t vpHeight = viewportDirty ? static_cast<uint32_t>(viewportHeight) : extent.height;
        
        // Flush all pending draw commands
        batchRenderer->flush(commandBuffer, renderPass->getHandle(), vpWidth, vpHeight);
        
        // End render pass
        renderPass->end(commandBuffer);
        
        // End command recording 
        commandBufferManager->endFrame();
        
        // Submit command buffer with proper semaphores
        std::vector<VkSemaphore> waitSemaphores = {swapchain->getImageAvailableSemaphore()};
        std::vector<VkPipelineStageFlags> waitStages = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        std::vector<VkSemaphore> signalSemaphores = {swapchain->getRenderFinishedSemaphore()};
        VkFence fence = swapchain->getInFlightFence();
        
        commandBufferManager->submit(device->getGraphicsQueue(), waitSemaphores, waitStages, signalSemaphores, fence);
        
        batchRenderer->endFrame();

        // Process deferred texture/resource deletions
        if (resourceManager) {
            resourceManager->processDeletes(*device);
        }
        
        swapchain->present(*device);
        currentFrame = (currentFrame + 1) % 2; // MAX_FRAMES_IN_FLIGHT = 2
        commandBufferManager->nextFrame();
    }

    // Convert StencilMode enum to pipeline stencil mode number
    static uint32_t stencilModeToPipelineMode(Renderer::StencilMode mode) {
        switch (mode) {
            case Renderer::StencilMode::WRITE:   return 1;
            case Renderer::StencilMode::TEST:    return 2;
            case Renderer::StencilMode::ERASE:   return 3;
            default:                              return 0; // DISABLE, CLEAR
        }
    }

    void VulkanRenderer::stampDrawState(DrawCommand& cmd) const {
        cmd.stencilMode = stencilModeToPipelineMode(currentStencilMode);
        cmd.scissorEnabled = scissorEnabled;
        cmd.scissorX = scissorX;
        cmd.scissorY = scissorY;
        cmd.scissorWidth = scissorWidth;
        cmd.scissorHeight = scissorHeight;
    }

    // Geometry rendering - implementations
    void VulkanRenderer::triangle(const Vector &p1, const Vector &p2, const Vector &p3, const Color &color) {
        if (!initialized || !batchRenderer || !inFrame) {
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
        stampDrawState(cmd);

        // Get pipeline based on current render state
        VulkanPipeline* pipeline = getStatePipeline(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
        if (pipeline) {
            batchRenderer->addDrawCommand(pipeline, cmd);
        } else {
            console->printLine("Warning: Pipeline not available for triangle rendering");
        }
    }

    void VulkanRenderer::triangle(const Vector &p1, const Vector &t1,
                                  const Vector &p2, const Vector &t2,
                                  const Vector &p3, const Vector &t3,
                                  const Material &material) {
        if (!initialized || !batchRenderer || !inFrame || !resourceManager) {
            return;
        }

        TextureId texId = material.getTexture();
        if (texId == INVALID_TEXTURE_ID) {
            return;
        }

        VkDescriptorSet texDescSet = resourceManager->getTextureDescriptorSet(texId);
        if (texDescSet == VK_NULL_HANDLE) {
            return;
        }

        // Build MaterialVertex data
        // Texture coords: x=u, y=v; z=layer index (texIndex)
        VertexFormats::MaterialVertex vertices[3] = {
            {p1, t1, 0},
            {p2, t2, 0},
            {p3, t3, 0}
        };

        DrawCommand cmd;
        cmd.type = DrawCommand::TRIANGLES;
        cmd.vertexCount = 3;
        cmd.vertices = vertices;
        cmd.vertexDataSize = sizeof(vertices);
        cmd.projectionMatrix = getProjectionMatrix();
        cmd.viewMatrix = getViewMatrix();
        cmd.modelMatrix = getModelMatrix();
        cmd.textureDescriptorSet = texDescSet;

        const Color& mc = material.getColor();
        cmd.modulateColor[0] = mc.getRed() / 255.0f;
        cmd.modulateColor[1] = mc.getGreen() / 255.0f;
        cmd.modulateColor[2] = mc.getBlue() / 255.0f;
        cmd.modulateColor[3] = mc.getAlpha() / 255.0f;
        cmd.alphaTest = material.isMasked() ? 1u : 0u;
        stampDrawState(cmd);

        VulkanPipeline* pipeline = getTextureStatePipeline(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
        if (pipeline) {
            batchRenderer->addDrawCommand(pipeline, cmd);
        }
    }

    void VulkanRenderer::quad(const Vector &p1, const Vector &p2, const Vector &p3, const Vector &p4, const Color &color) {
        if (!initialized || !batchRenderer || !inFrame) {
            return;
        }

        // Convert color to float values
        float r = color.getRed() / 255.0f;
        float g = color.getGreen() / 255.0f;
        float b = color.getBlue() / 255.0f;
        float a = color.getAlpha() / 255.0f;

        // A quad is rendered as two triangles (6 vertices)
        // Triangle 1: p1, p2, p3
        // Triangle 2: p1, p3, p4
        VertexFormats::ColorVertex vertices[6] = {
            // Triangle 1
            {p1, {r, g, b, a}},
            {p2, {r, g, b, a}},
            {p3, {r, g, b, a}},
            // Triangle 2
            {p1, {r, g, b, a}},
            {p3, {r, g, b, a}},
            {p4, {r, g, b, a}}
        };

        // Create draw command
        DrawCommand cmd;
        cmd.type = DrawCommand::TRIANGLES;
        cmd.vertexCount = 6;
        cmd.vertices = vertices;
        cmd.vertexDataSize = sizeof(vertices);
        cmd.projectionMatrix = getProjectionMatrix();
        cmd.viewMatrix = getViewMatrix();
        cmd.modelMatrix = getModelMatrix();
        cmd.color = color;
        stampDrawState(cmd);

        // Get pipeline based on current render state
        VulkanPipeline* pipeline = getStatePipeline(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
        if (pipeline) {
            batchRenderer->addDrawCommand(pipeline, cmd);
        } else {
            console->printLine("Warning: Pipeline not available for quad rendering");
        }
    }

    void VulkanRenderer::quad(const Vector &p1, const Vector &t1,
                              const Vector &p2, const Vector &t2,
                              const Vector &p3, const Vector &t3,
                              const Vector &p4, const Vector &t4,
                              const Material &material) {
        if (!initialized || !batchRenderer || !inFrame || !resourceManager) {
            return;
        }

        TextureId texId = material.getTexture();
        if (texId == INVALID_TEXTURE_ID) {
            return;
        }

        VkDescriptorSet texDescSet = resourceManager->getTextureDescriptorSet(texId);
        if (texDescSet == VK_NULL_HANDLE) {
            return;
        }

        // A quad is rendered as two triangles (6 vertices)
        VertexFormats::MaterialVertex vertices[6] = {
            // Triangle 1: p1, p2, p3
            {p1, t1, 0},
            {p2, t2, 0},
            {p3, t3, 0},
            // Triangle 2: p1, p3, p4
            {p1, t1, 0},
            {p3, t3, 0},
            {p4, t4, 0}
        };

        DrawCommand cmd;
        cmd.type = DrawCommand::TRIANGLES;
        cmd.vertexCount = 6;
        cmd.vertices = vertices;
        cmd.vertexDataSize = sizeof(vertices);
        cmd.projectionMatrix = getProjectionMatrix();
        cmd.viewMatrix = getViewMatrix();
        cmd.modelMatrix = getModelMatrix();
        cmd.textureDescriptorSet = texDescSet;

        const Color& mc = material.getColor();
        cmd.modulateColor[0] = mc.getRed() / 255.0f;
        cmd.modulateColor[1] = mc.getGreen() / 255.0f;
        cmd.modulateColor[2] = mc.getBlue() / 255.0f;
        cmd.modulateColor[3] = mc.getAlpha() / 255.0f;
        cmd.alphaTest = material.isMasked() ? 1u : 0u;
        stampDrawState(cmd);

        VulkanPipeline* pipeline = getTextureStatePipeline(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
        if (pipeline) {
            batchRenderer->addDrawCommand(pipeline, cmd);
        }
    }

    void VulkanRenderer::point(const Vector &position, Float32 size, const Color &color) {
        if (!initialized || !batchRenderer || !inFrame) {
            return;
        }

        float r = color.getRed() / 255.0f;
        float g = color.getGreen() / 255.0f;
        float b = color.getBlue() / 255.0f;
        float a = color.getAlpha() / 255.0f;

        VertexFormats::ColorVertex vertices[1] = {
            {position, {r, g, b, a}}
        };

        DrawCommand cmd;
        cmd.type = DrawCommand::POINTS;
        cmd.vertexCount = 1;
        cmd.vertices = vertices;
        cmd.vertexDataSize = sizeof(vertices);
        cmd.projectionMatrix = getProjectionMatrix();
        cmd.viewMatrix = getViewMatrix();
        cmd.modelMatrix = getModelMatrix();
        cmd.color = color;
        stampDrawState(cmd);

        VulkanPipeline* pipeline = getStatePipeline(VK_PRIMITIVE_TOPOLOGY_POINT_LIST);
        if (pipeline) {
            batchRenderer->addDrawCommand(pipeline, cmd);
        }
    }

    void VulkanRenderer::line(const Vector &from, const Vector &to, Float32 width, const Color &color) {
        if (!initialized || !batchRenderer || !inFrame) {
            return;
        }

        float r = color.getRed() / 255.0f;
        float g = color.getGreen() / 255.0f;
        float b = color.getBlue() / 255.0f;
        float a = color.getAlpha() / 255.0f;

        VertexFormats::ColorVertex vertices[2] = {
            {from, {r, g, b, a}},
            {to,   {r, g, b, a}}
        };

        DrawCommand cmd;
        cmd.type = DrawCommand::LINES;
        cmd.vertexCount = 2;
        cmd.vertices = vertices;
        cmd.vertexDataSize = sizeof(vertices);
        cmd.projectionMatrix = getProjectionMatrix();
        cmd.viewMatrix = getViewMatrix();
        cmd.modelMatrix = getModelMatrix();
        cmd.color = color;
        stampDrawState(cmd);

        VulkanPipeline* pipeline = getStatePipeline(VK_PRIMITIVE_TOPOLOGY_LINE_LIST);
        if (pipeline) {
            batchRenderer->addDrawCommand(pipeline, cmd);
        }
    }

    void VulkanRenderer::lines(const Vector positions[4], Float32 width, const Color &color) {
        if (!initialized || !batchRenderer || !inFrame) {
            return;
        }

        float r = color.getRed() / 255.0f;
        float g = color.getGreen() / 255.0f;
        float b = color.getBlue() / 255.0f;
        float a = color.getAlpha() / 255.0f;

        VertexFormats::ColorVertex vertices[4] = {
            {positions[0], {r, g, b, a}},
            {positions[1], {r, g, b, a}},
            {positions[2], {r, g, b, a}},
            {positions[3], {r, g, b, a}}
        };

        DrawCommand cmd;
        cmd.type = DrawCommand::LINE_STRIP;
        cmd.vertexCount = 4;
        cmd.vertices = vertices;
        cmd.vertexDataSize = sizeof(vertices);
        cmd.projectionMatrix = getProjectionMatrix();
        cmd.viewMatrix = getViewMatrix();
        cmd.modelMatrix = getModelMatrix();
        cmd.color = color;
        stampDrawState(cmd);

        VulkanPipeline* pipeline = getStatePipeline(VK_PRIMITIVE_TOPOLOGY_LINE_STRIP);
        if (pipeline) {
            batchRenderer->addDrawCommand(pipeline, cmd);
        }
    }

    void VulkanRenderer::lines(const std::vector<Vector> &positions, const std::vector<int>& indices, 
                               const int restartIndex, Float32 width, const Color &color) {
        if (!initialized || !batchRenderer || !inFrame) {
            return;
        }

        float r = color.getRed() / 255.0f;
        float g = color.getGreen() / 255.0f;
        float b = color.getBlue() / 255.0f;
        float a = color.getAlpha() / 255.0f;

        // Build line segments from indexed line strip, splitting at restart indices
        // Each non-restart segment becomes a LINE_STRIP draw command
        std::vector<VertexFormats::ColorVertex> segmentVertices;
        VulkanPipeline* pipeline = getStatePipeline(VK_PRIMITIVE_TOPOLOGY_LINE_STRIP);
        if (!pipeline) return;

        for (size_t i = 0; i < indices.size(); i++) {
            if (indices[i] == restartIndex) {
                // Flush current segment
                if (segmentVertices.size() >= 2) {
                    DrawCommand cmd;
                    cmd.type = DrawCommand::LINE_STRIP;
                    cmd.vertexCount = static_cast<uint32_t>(segmentVertices.size());
                    cmd.vertices = segmentVertices.data();
                    cmd.vertexDataSize = segmentVertices.size() * sizeof(VertexFormats::ColorVertex);
                    cmd.projectionMatrix = getProjectionMatrix();
                    cmd.viewMatrix = getViewMatrix();
                    cmd.modelMatrix = getModelMatrix();
                    cmd.color = color;
                    stampDrawState(cmd);
                    batchRenderer->addDrawCommand(pipeline, cmd);
                }
                segmentVertices.clear();
            } else {
                int idx = indices[i];
                if (idx >= 0 && idx < static_cast<int>(positions.size())) {
                    segmentVertices.push_back({positions[idx], {r, g, b, a}});
                }
            }
        }
        // Flush final segment
        if (segmentVertices.size() >= 2) {
            DrawCommand cmd;
            cmd.type = DrawCommand::LINE_STRIP;
            cmd.vertexCount = static_cast<uint32_t>(segmentVertices.size());
            cmd.vertices = segmentVertices.data();
            cmd.vertexDataSize = segmentVertices.size() * sizeof(VertexFormats::ColorVertex);
            cmd.projectionMatrix = getProjectionMatrix();
            cmd.viewMatrix = getViewMatrix();
            cmd.modelMatrix = getModelMatrix();
            cmd.color = color;
            stampDrawState(cmd);
            batchRenderer->addDrawCommand(pipeline, cmd);
        }
    }

    void VulkanRenderer::stencil(StencilMode mode) const {
        currentStencilMode = mode;
        
        if (mode == StencilMode::CLEAR && batchRenderer && inFrame) {
            // Insert a special "clear stencil" command into the draw stream
            // This will be translated to vkCmdClearAttachments during flush
            DrawCommand clearCmd;
            clearCmd.stencilMode = 4; // Special: clear stencil
            clearCmd.vertexCount = 0;
            clearCmd.vertices = nullptr;
            clearCmd.vertexDataSize = 0;
            
            // Use whatever pipeline is available (the command won't actually draw)
            VulkanPipeline* pipeline = const_cast<VulkanRenderer*>(this)->getStatePipeline(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
            if (pipeline) {
                batchRenderer->addDrawCommand(pipeline, clearCmd);
            }
        }
    }

    bool VulkanRenderer::setScissorTest(bool value) const {
        bool previous = scissorEnabled;
        scissorEnabled = value;
        return previous;
    }

    void VulkanRenderer::scissorTest(Int32 x, Int32 y, Int32 width, Int32 height) const {
        scissorX = x;
        // Vulkan Y-axis is top-down, OpenGL is bottom-up
        // Convert from OpenGL-style (origin at bottom-left) to Vulkan-style (origin at top-left)
        if (swapchain) {
            VkExtent2D extent = swapchain->getExtent();
            scissorY = static_cast<Int32>(extent.height) - y - height;
        } else {
            scissorY = y;
        }
        scissorWidth = width;
        scissorHeight = height;
    }

    // ============================================================
    // VulkanRendererBuffer - static world geometry buffer
    // ============================================================
    
    class VulkanRendererBuffer : public RendererBuffer {
    private:
        VulkanRenderer& renderer;
        size_t faceCount;
        
        // Base vertex data: triangulated quads (6 verts per face), constant after construction
        struct BaseVertex {
            float x, y, z;
            float u, v;
            uint32_t flags;
        };
        std::vector<BaseVertex> baseVertices;
        
        // Per-face texture layer index (updated via update())
        std::vector<float> textureLayers;
        
        // Full MaterialVertex array, rebuilt when texture layers change
        std::vector<VertexFormats::MaterialVertex> materialVertices;
        bool dirty;
        
    public:
        VulkanRendererBuffer(VulkanRenderer& renderer, const FaceList& faceList)
            : renderer(renderer)
            , faceCount(faceList.getFaces().size())
            , dirty(true) {
            
            // Triangulate quads: 4 vertices per face → 6 triangle vertices per face
            const Vertex* vertex = faceList.getVertexes().data();
            const auto& faces = faceList.getFaces();
            
            baseVertices.reserve(faceCount * 6);
            textureLayers.reserve(faceCount);
            
            for (size_t i = 0; i < faceCount; i++, vertex += 4) {
                const Vertex& v1 = vertex[0];
                const Vertex& v2 = vertex[1];
                const Vertex& v3 = vertex[2];
                const Vertex& v4 = vertex[3];
                
                // Triangle 1: v1, v2, v3
                baseVertices.push_back({v1.x, v1.y, v1.z, v1.u, v1.v, v1.getFlag()});
                baseVertices.push_back({v2.x, v2.y, v2.z, v2.u, v2.v, v2.getFlag()});
                baseVertices.push_back({v3.x, v3.y, v3.z, v3.u, v3.v, v3.getFlag()});
                // Triangle 2: v1, v3, v4
                baseVertices.push_back({v1.x, v1.y, v1.z, v1.u, v1.v, v1.getFlag()});
                baseVertices.push_back({v3.x, v3.y, v3.z, v3.u, v3.v, v3.getFlag()});
                baseVertices.push_back({v4.x, v4.y, v4.z, v4.u, v4.v, v4.getFlag()});
                
                textureLayers.push_back(static_cast<float>(faces[i].getCurrentTexture()));
            }
            
            materialVertices.resize(faceCount * 6);
            rebuildMaterialVertices();
        }
        
        void update(const FaceList& faceList) override {
            const auto& faces = faceList.getFaces();
            for (size_t i = 0; i < faceCount && i < faces.size(); i++) {
                textureLayers[i] = static_cast<float>(faces[i].getCurrentTexture());
            }
            dirty = true;
            rebuildMaterialVertices();
        }
        
        void render(const Material& material) override {
            if (faceCount == 0 || materialVertices.empty()) return;
            
            renderer.renderMaterialVertices(
                materialVertices.data(),
                static_cast<uint32_t>(materialVertices.size()),
                materialVertices.size() * sizeof(VertexFormats::MaterialVertex),
                material);
        }
        
    private:
        void rebuildMaterialVertices() {
            for (size_t face = 0; face < faceCount; face++) {
                float layer = textureLayers[face];
                for (size_t v = 0; v < 6; v++) {
                    size_t idx = face * 6 + v;
                    const BaseVertex& bv = baseVertices[idx];
                    materialVertices[idx].position = Vector(bv.x, bv.y, bv.z);
                    materialVertices[idx].textureCoord = Vector(bv.u, bv.v, layer);
                    materialVertices[idx].flags = bv.flags;
                }
            }
            dirty = false;
        }
    };

    std::unique_ptr<RendererBuffer> VulkanRenderer::makeBuffer(const FaceList &faceList) {
        if (faceList.getFaces().empty()) {
            return nullptr;
        }
        return std::make_unique<VulkanRendererBuffer>(*this, faceList);
    }

    void VulkanRenderer::renderMaterialVertices(const void* vertices, uint32_t vertexCount,
                                                 size_t vertexDataSize, const Material& material) {
        if (!initialized || !batchRenderer || !inFrame || !resourceManager) {
            return;
        }

        TextureId texId = material.getTexture();
        if (texId == INVALID_TEXTURE_ID) {
            return;
        }

        VkDescriptorSet texDescSet = resourceManager->getTextureDescriptorSet(texId);
        if (texDescSet == VK_NULL_HANDLE) {
            return;
        }

        DrawCommand cmd;
        cmd.type = DrawCommand::TRIANGLES;
        cmd.vertexCount = vertexCount;
        cmd.vertices = vertices;
        cmd.vertexDataSize = vertexDataSize;
        cmd.projectionMatrix = getProjectionMatrix();
        cmd.viewMatrix = getViewMatrix();
        cmd.modelMatrix = getModelMatrix();
        cmd.textureDescriptorSet = texDescSet;

        const Color& mc = material.getColor();
        cmd.modulateColor[0] = mc.getRed() / 255.0f;
        cmd.modulateColor[1] = mc.getGreen() / 255.0f;
        cmd.modulateColor[2] = mc.getBlue() / 255.0f;
        cmd.modulateColor[3] = mc.getAlpha() / 255.0f;
        cmd.alphaTest = material.isMasked() ? 1u : 0u;
        stampDrawState(cmd);

        VulkanPipeline* pipeline = getTextureStatePipeline(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
        if (pipeline) {
            batchRenderer->addDrawCommand(pipeline, cmd);
        }
    }

    std::unique_ptr<RendererTarget> VulkanRenderer::makeTarget(ScreenParameters screenParameters) {
        return std::make_unique<VulkanRendererTarget>(*this, screenParameters);
    }

    void VulkanRenderer::flushAndEndRenderPass(VkCommandBuffer cmdBuf) {
        if (!inFrame || !batchRenderer || !renderPass) {
            return;
        }

        VkExtent2D extent = swapchain->getExtent();
        uint32_t vpWidth = viewportDirty ? static_cast<uint32_t>(viewportWidth) : extent.width;
        uint32_t vpHeight = viewportDirty ? static_cast<uint32_t>(viewportHeight) : extent.height;

        batchRenderer->flush(cmdBuf, renderPass->getHandle(), vpWidth, vpHeight);
        renderPass->end(cmdBuf);
    }

    void VulkanRenderer::beginMainRenderPass(VkCommandBuffer cmdBuf) {
        if (!renderPass || !swapchain) {
            return;
        }

        // Restart the main render pass without clearing (use LOAD instead of CLEAR)
        uint32_t framebufferIndex = swapchain->getCurrentImageIndex();

        VkRenderPassBeginInfo rpInfo{};
        rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        rpInfo.renderPass = renderPass->getHandle();
        rpInfo.framebuffer = renderPass->getFramebuffer(framebufferIndex);
        rpInfo.renderArea.offset = {0, 0};
        rpInfo.renderArea.extent = {renderPass->getWidth(), renderPass->getHeight()};

        // We don't want to clear since content was already rendered before the interruption
        // But the render pass was created with LOAD_OP_CLEAR. The clear values are ignored
        // when loadOp is CLEAR and the render pass is restarted - the content from before
        // vkCmdEndRenderPass is already in the attachments. However, with initialLayout=UNDEFINED
        // and loadOp=CLEAR, restarting will clear again.
        //
        // To avoid this, we pass clear values but accept the clear. The content that was
        // rendered before the offscreen pass is already submitted. The main render pass
        // restart will clear, but that's acceptable because in the current game flow,
        // rerender() is called from prerender() (before clearBuffers), and apply() just
        // draws a quad during the active frame.
        std::vector<VkClearValue> clearValues;
        VkClearValue colorClear{};
        colorClear.color = renderPass->getConfig().clearColorValue;
        clearValues.push_back(colorClear);
        if (renderPass->getConfig().hasDepthBuffer) {
            VkClearValue depthClear{};
            depthClear.depthStencil = {renderPass->getConfig().clearDepthValue, renderPass->getConfig().stencilClearValue};
            clearValues.push_back(depthClear);
        }

        rpInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        rpInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(cmdBuf, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);
    }

    void VulkanRenderer::createPipelines() {
        if (!initialized || !pipelineCache || !renderPass) {
            return;
        }

        VkRenderPass rp = renderPass->getHandle();

        // Helper to register a pipeline in the cache
        auto cachePipeline = [&](const char* name, std::unique_ptr<VulkanPipeline> ptr) -> VulkanPipeline* {
            if (!ptr) return nullptr;
            size_t hash = std::hash<std::string>{}(name);
            return pipelineCache->getOrCreatePipeline(hash, [&]() { return std::move(ptr); });
        };

        try {
            colorPipeline = cachePipeline("color_pipeline",
                PipelinePresets::createSimpleColorPipeline(*device, rp));
        } catch (const std::exception& e) {
            console->printLine(Format("Failed to create color pipeline: {0}") << e.what());
        }

        try {
            linePipeline = cachePipeline("line_pipeline",
                PipelinePresets::createSimpleLinePipeline(*device, rp));
        } catch (const std::exception& e) {
            console->printLine(Format("Failed to create line pipeline: {0}") << e.what());
        }

        try {
            lineStripPipeline = cachePipeline("line_strip_pipeline",
                PipelinePresets::createSimpleLineStripPipeline(*device, rp));
        } catch (const std::exception& e) {
            console->printLine(Format("Failed to create line-strip pipeline: {0}") << e.what());
        }

        try {
            pointPipeline = cachePipeline("point_pipeline",
                PipelinePresets::createSimplePointPipeline(*device, rp));
        } catch (const std::exception& e) {
            console->printLine(Format("Failed to create point pipeline: {0}") << e.what());
        }
    }

    VulkanPipeline* VulkanRenderer::getColorPipeline() {
        if (!colorPipeline) {
            createPipelines();
        }
        return colorPipeline;
    }

    VulkanPipeline* VulkanRenderer::getLinePipeline() {
        if (!linePipeline) {
            createPipelines();
        }
        return linePipeline;
    }

    VulkanPipeline* VulkanRenderer::getLineStripPipeline() {
        if (!lineStripPipeline) {
            createPipelines();
        }
        return lineStripPipeline;
    }

    VulkanPipeline* VulkanRenderer::getPointPipeline() {
        if (!pointPipeline) {
            createPipelines();
        }
        return pointPipeline;
    }

    VulkanPipeline* VulkanRenderer::getTexturePipeline() {
        if (!texturePipeline) {
            createPipelines();
        }
        return texturePipeline;
    }

    VulkanPipeline* VulkanRenderer::getStatePipeline(VkPrimitiveTopology topology) {
        if (!pipelineCache || !renderPass) {
            return nullptr;
        }

        uint32_t sm = stencilModeToPipelineMode(currentStencilMode);

        // Compute a hash key from (topology, depthTest, depthWrite, blendFunc, wireframe, stencilMode)
        uint32_t key = static_cast<uint32_t>(topology);
        key |= (depthTestEnabled  ? 1u : 0u) << 8;
        key |= (depthWriteEnabled ? 1u : 0u) << 9;
        key |= static_cast<uint32_t>(currentBlendFunc) << 10;
        key |= (wireframeEnabled  ? 1u : 0u) << 12;
        key |= sm << 13; // Stencil mode in bits 13-14
        
        size_t hash = std::hash<uint32_t>{}(key);

        VkRenderPass rp = renderPass->getHandle();
        bool dt = depthTestEnabled;
        bool dw = depthWriteEnabled;
        BlendFunc bf = currentBlendFunc;
        bool wf = wireframeEnabled;

        return pipelineCache->getOrCreatePipeline(hash, [&]() {
            return PipelinePresets::createSimpleColorPipelineWithState(
                *device, rp, topology, dt, dw, bf, wf, sm);
        });
    }

    VulkanPipeline* VulkanRenderer::getTextureStatePipeline(VkPrimitiveTopology topology) {
        if (!pipelineCache || !renderPass || textureDescriptorSetLayout == VK_NULL_HANDLE) {
            return nullptr;
        }

        uint32_t sm = stencilModeToPipelineMode(currentStencilMode);

        // Compute hash key differentiated from color pipelines (set bit 16 for texture)
        uint32_t key = static_cast<uint32_t>(topology);
        key |= (depthTestEnabled  ? 1u : 0u) << 8;
        key |= (depthWriteEnabled ? 1u : 0u) << 9;
        key |= static_cast<uint32_t>(currentBlendFunc) << 10;
        key |= (wireframeEnabled  ? 1u : 0u) << 12;
        key |= sm << 13; // Stencil mode in bits 13-14
        key |= 1u << 16; // Texture pipeline flag

        size_t hash = std::hash<uint32_t>{}(key);

        VkRenderPass rp = renderPass->getHandle();
        VkDescriptorSetLayout texLayout = textureDescriptorSetLayout;
        bool dt = depthTestEnabled;
        bool dw = depthWriteEnabled;
        BlendFunc bf = currentBlendFunc;
        bool wf = wireframeEnabled;

        return pipelineCache->getOrCreatePipeline(hash, [&]() {
            return PipelinePresets::createSimpleTexturePipelineWithState(
                *device, rp, texLayout, topology, dt, dw, bf, wf, sm);
        });
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