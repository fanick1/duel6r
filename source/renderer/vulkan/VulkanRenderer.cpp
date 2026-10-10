#include "VulkanRenderer.h"
#include <array>
#include <SDL2/SDL_vulkan.h>
#include "../../Format.h"
#include "../../FaceList.h"
#include "../../Vertex.h"

namespace Duel6 {

    namespace {
        uint32_t packColor(const Color& color) {
            return uint32_t(color.getRed()) | uint32_t(color.getGreen()) << 8 |
                   uint32_t(color.getBlue()) << 16 | uint32_t(color.getAlpha()) << 24;
        }

        VulkanVertex vertex(const Vector& position, const Color& color) {
            return {position, Vector(0, 0, 0), packColor(color), 0};
        }

        VulkanVertex vertex(const Vector& position, const Vector& textureCoord) {
            return {position, textureCoord, packColor(Color::WHITE), 0};
        }

        // The game's projections map depth to OpenGL's [-1, 1]; Vulkan clips to [0, 1]
        Matrix openGLToVulkanClipSpace() {
            Matrix m = Matrix::IDENTITY;
            m.getStorage()[10] = 0.5f;
            m.getStorage()[14] = 0.5f;
            return m;
        }
    }

    VulkanRenderer::VulkanRenderer(SDL_Window* window, Console& console)
        : window(window)
        , console(&console)
        , initialized(false)
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
            console->printLine(Format("[vulkan] swapchain created: {0}x{1}")
                               << swapchain->getExtent().width << swapchain->getExtent().height);
            viewportWidth = static_cast<Int32>(swapchain->getExtent().width);
            viewportHeight = static_cast<Int32>(swapchain->getExtent().height);

            // Initialize rendering components
            descriptorManager = std::make_unique<VulkanDescriptorManager>(*console);
            descriptorManager->initialize();

            // Initialize resource management
            samplerManager = std::make_unique<VulkanSamplerManager>(*console);
            resourceManager = std::make_unique<VulkanResourceManager>(*console);
            resourceManager->initialize(memoryManager.get(), samplerManager.get(), descriptorManager.get());

            // Cache the texture descriptor set layout for pipeline creation
            textureDescriptorSetLayout = descriptorManager->getTextureLayout(*device);

            Image white(1, 1);
            white.at(0) = Color::WHITE;
            whiteTexture = resourceManager->getTextureDescriptorSet(
                resourceManager->createTexture(*device, white, TextureFilter::Nearest, true));

            batchRenderer = std::make_unique<VulkanBatchRenderer>(*device, *memoryManager, *console);
            
            // Create render pass with default configuration
            RenderPassConfig renderPassConfig;
            renderPassConfig.colorFormat = swapchain->getImageFormat();
            renderPassConfig.clearColorValue = {{0.0f, 0.0f, 0.0f, 1.0f}}; // Black
            renderPass = std::make_unique<VulkanRenderPass>(*device, renderPassConfig);
            
            // Create framebuffers for the render pass
            renderPass->createFramebuffers(*swapchain);
            
            commandBufferManager = std::make_unique<VulkanCommandBufferManager>(*device, 2);
            
            // Initialize pipeline cache and create basic pipelines
            pipelineCache = std::make_unique<VulkanPipelineCache>(*device);
            
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
            destroyCaptureImage();
        }

        // Cleanup rendering components
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
        SDL_Vulkan_GetDrawableSize(window, &width, &height);
        while (width == 0 || height == 0) {
            SDL_Vulkan_GetDrawableSize(window, &width, &height);
            SDL_WaitEvent(nullptr);
        }

        device->waitIdle();
        destroyCaptureImage();
        
        // Clear batches BEFORE destroying pipelines to avoid dangling pipeline pointers
        if (batchRenderer) {
            batchRenderer->discard();
        }
        
        swapchain->recreate(*device, *surface);
        console->printLine(Format("[vulkan] swapchain recreated: {0}x{1} (window {2}x{3})")
                           << swapchain->getExtent().width << swapchain->getExtent().height << width << height);
        
                // The surface format does not change on resize, only the framebuffers do
        renderPass->recreateFramebuffers(*swapchain);
        
        // Recreate pipelines (they reference the render pass)
        pipelineCache = std::make_unique<VulkanPipelineCache>(*device);
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

    void VulkanRenderer::ensureCaptureImage(VkExtent2D extent) {
                // recreateSwapchain() destroys the capture image, so an existing one always has the right size
        if (captureImage != VK_NULL_HANDLE) {
            return;
        }

        VkDevice dev = device->getLogicalDevice();

        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.format = swapchain->getImageFormat();
        imageInfo.extent = {extent.width, extent.height, 1};
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        if (vkCreateImage(dev, &imageInfo, nullptr, &captureImage) != VK_SUCCESS) {
            D6_THROW(VulkanException, "Failed to create frame capture image");
        }

        VkMemoryRequirements memReqs;
        vkGetImageMemoryRequirements(dev, captureImage, &memReqs);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memReqs.size;
        allocInfo.memoryTypeIndex = device->findMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        if (vkAllocateMemory(dev, &allocInfo, nullptr, &captureMemory) != VK_SUCCESS) {
            vkDestroyImage(dev, captureImage, nullptr);
            captureImage = VK_NULL_HANDLE;
            D6_THROW(VulkanException, "Failed to allocate frame capture memory");
        }
        vkBindImageMemory(dev, captureImage, captureMemory, 0);

        captureExtent = extent;
        captureValid = false;
    }

    void VulkanRenderer::destroyCaptureImage() {
        VkDevice dev = device->getLogicalDevice();
        if (captureImage != VK_NULL_HANDLE) {
            vkDestroyImage(dev, captureImage, nullptr);
            captureImage = VK_NULL_HANDLE;
        }
        if (captureMemory != VK_NULL_HANDLE) {
            vkFreeMemory(dev, captureMemory, nullptr);
            captureMemory = VK_NULL_HANDLE;
        }
        captureExtent = {};
        captureValid = false;
    }

    void VulkanRenderer::recordFrameCapture(VkCommandBuffer commandBuffer) {
        VkExtent2D extent = swapchain->getExtent();
        ensureCaptureImage(extent);

        VkImage swapchainImage = swapchain->getImage(swapchain->getCurrentImageIndex());

        VkImageSubresourceRange range{};
        range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        range.levelCount = 1;
        range.layerCount = 1;

        // Swapchain image: PRESENT_SRC (render pass final layout) -> TRANSFER_SRC
        // Capture image: previous contents discarded -> TRANSFER_DST
        std::array<VkImageMemoryBarrier, 2> before{};
        before[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        before[0].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        before[0].dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        before[0].oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        before[0].newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        before[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        before[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        before[0].image = swapchainImage;
        before[0].subresourceRange = range;

        before[1].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        before[1].srcAccessMask = 0;
        before[1].dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        before[1].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        before[1].newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        before[1].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        before[1].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        before[1].image = captureImage;
        before[1].subresourceRange = range;

        vkCmdPipelineBarrier(commandBuffer,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            0, 0, nullptr, 0, nullptr, static_cast<uint32_t>(before.size()), before.data());

        VkImageCopy region{};
        region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.srcSubresource.layerCount = 1;
        region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.dstSubresource.layerCount = 1;
        region.extent = {extent.width, extent.height, 1};

        vkCmdCopyImage(commandBuffer,
            swapchainImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            captureImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &region);

        // Swapchain image back to PRESENT_SRC; capture image stays in TRANSFER_SRC for readback
        std::array<VkImageMemoryBarrier, 2> after = before;
        after[0].srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        after[0].dstAccessMask = 0;
        after[0].oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        after[0].newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        after[1].srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        after[1].dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        after[1].oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        after[1].newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

        vkCmdPipelineBarrier(commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
            0, 0, nullptr, 0, nullptr, static_cast<uint32_t>(after.size()), after.data());

        captureValid = true;
    }

    Image VulkanRenderer::makeScreenshot() {
        if (!initialized || !device || !swapchain || !memoryManager || !captureValid) {
            return Image();
        }

        device->waitIdle();

        uint32_t width = captureExtent.width;
        uint32_t height = captureExtent.height;
        VkDeviceSize imageSize = static_cast<VkDeviceSize>(width) * height * 4; // RGBA 8-bit

        // Host-visible buffer the capture image is copied into
        VulkanBuffer readbackBuffer = memoryManager->createBuffer(*device, imageSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        VkCommandBuffer cmdBuf = device->beginSingleTimeCommands();

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

        vkCmdCopyImageToBuffer(cmdBuf, captureImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                               readbackBuffer.buffer, 1, &region);

        VkBufferMemoryBarrier hostBarrier{};
        hostBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        hostBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        hostBarrier.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
        hostBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        hostBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        hostBarrier.buffer = readbackBuffer.buffer;
        hostBarrier.offset = 0;
        hostBarrier.size = VK_WHOLE_SIZE;

        vkCmdPipelineBarrier(cmdBuf,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT,
            0, 0, nullptr, 1, &hostBarrier, 0, nullptr);

        device->endSingleTimeCommands(cmdBuf);

        // Read pixels from readback buffer
        void* data = memoryManager->mapBuffer(*device, readbackBuffer);
        if (!data) {
            memoryManager->destroyBuffer(*device, readbackBuffer);
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

        memoryManager->unmapBuffer(*device, readbackBuffer);
        memoryManager->destroyBuffer(*device, readbackBuffer);

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

        // Not every platform reports a resize through VK_ERROR_OUT_OF_DATE_KHR (Wayland never does),
        // so compare the drawable size with the swapchain ourselves
        int drawableWidth = 0, drawableHeight = 0;
        SDL_Vulkan_GetDrawableSize(window, &drawableWidth, &drawableHeight);
        VkExtent2D extent = swapchain->getExtent();
        if (static_cast<uint32_t>(drawableWidth) != extent.width ||
            static_cast<uint32_t>(drawableHeight) != extent.height) {
            recreateSwapchain();
        }

        if (!swapchain->acquireNextImage(*device)) {
            // Out of date - recreate and try once more, otherwise skip this frame
            recreateSwapchain();
            if (!swapchain->acquireNextImage(*device)) {
                return;
            }
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
        
        VkExtent2D extent = swapchain->getExtent();
        batchRenderer->flush(commandBuffer, extent.width, extent.height, true);

        // End render pass
        renderPass->end(commandBuffer);

        // Keep a copy of the frame for screenshots while the swapchain image is still acquired
        recordFrameCapture(commandBuffer);
        
        // End command recording 
        commandBufferManager->endFrame();
        
        // Submit command buffer with proper semaphores
        std::vector<VkSemaphore> waitSemaphores = {swapchain->getImageAvailableSemaphore()};
        std::vector<VkPipelineStageFlags> waitStages = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        std::vector<VkSemaphore> signalSemaphores = {swapchain->getRenderFinishedSemaphore()};
        VkFence fence = swapchain->getInFlightFence();
        
        commandBufferManager->submit(device->getGraphicsQueue(), waitSemaphores, waitStages, signalSemaphores, fence);
        
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


    // Geometry rendering - implementations








    void VulkanRenderer::stencil(StencilMode mode) const {
        currentStencilMode = mode;
        
        if (mode == StencilMode::CLEAR && batchRenderer && inFrame) {
            batchRenderer->clearStencil();
        }
    }

    bool VulkanRenderer::setScissorTest(bool value) const {
        bool previous = scissorEnabled;
        scissorEnabled = value;
        return previous;
    }

    void VulkanRenderer::scissorTest(Int32 x, Int32 y, Int32 width, Int32 height) const {
        scissorX = x;
        scissorY = y;
        scissorWidth = width;
        scissorHeight = height;
    }

    // ============================================================
    // VulkanRendererBuffer - static world geometry buffer
    // ============================================================
    
    // Static level geometry: the faces are triangulated once, update() only changes the texture layers
    class VulkanRendererBuffer : public RendererBuffer {
    private:
        VulkanRenderer& renderer;
        std::vector<VulkanVertex> vertices;  // 6 per face

    public:
        VulkanRendererBuffer(VulkanRenderer& renderer, const FaceList& faceList) : renderer(renderer) {
            const Vertex* corner = faceList.getVertexes().data();
            vertices.reserve(faceList.getFaces().size() * 6);
            for (const Face& face : faceList.getFaces()) {
                Float32 layer = static_cast<Float32>(face.getCurrentTexture());
                for (Size index : {0, 1, 2, 0, 2, 3}) {
                    const Vertex& v = corner[index];
                    vertices.push_back({Vector(v.x, v.y, v.z), Vector(v.u, v.v, layer), packColor(Color::WHITE), v.getFlag()});
                }
                corner += 4;
            }
        }

        void update(const FaceList& faceList) override {
            const auto& faces = faceList.getFaces();
            for (size_t face = 0; face < faces.size() && face * 6 < vertices.size(); face++) {
                Float32 layer = static_cast<Float32>(faces[face].getCurrentTexture());
                for (size_t i = 0; i < 6; i++) {
                    vertices[face * 6 + i].textureCoord.z = layer;
                }
            }
        }

        void render(const Material& material) override {
            renderer.submitVertices(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, vertices.data(),
                                    static_cast<uint32_t>(vertices.size()), &material);
        }
    };

    std::unique_ptr<RendererBuffer> VulkanRenderer::makeBuffer(const FaceList &faceList) {
        if (faceList.getFaces().empty()) {
            return nullptr;
        }
        return std::make_unique<VulkanRendererBuffer>(*this, faceList);
    }


    std::unique_ptr<RendererTarget> VulkanRenderer::makeTarget(ScreenParameters screenParameters) {
        return std::make_unique<VulkanRendererTarget>(*this, screenParameters);
    }












    DrawCommand::State VulkanRenderer::makeDrawState(VkPrimitiveTopology topology) const {
        DrawCommand::State state;
        state.topology = topology;
        state.texture = whiteTexture;
        state.time = globalTime;
        state.stencilMode = stencilModeToPipelineMode(currentStencilMode);
        state.viewport[0] = viewportX;
        state.viewport[1] = viewportY;
        state.viewport[2] = viewportWidth;
        state.viewport[3] = viewportHeight;
        state.scissorEnabled = scissorEnabled;
        state.scissor[0] = scissorX;
        state.scissor[1] = scissorY;
        state.scissor[2] = scissorWidth;
        state.scissor[3] = scissorHeight;
        static const Matrix clipSpace = openGLToVulkanClipSpace();
        state.mvp = clipSpace * projectionMatrix * viewMatrix * modelMatrix;
        return state;
    }

    void VulkanRenderer::submitVertices(VkPrimitiveTopology topology, const VulkanVertex* vertices, uint32_t vertexCount,
                                        const Material* material) {
        if (!initialized || !inFrame) {
            return;
        }

        DrawCommand::State state = makeDrawState(topology);
        if (material != nullptr) {
            state.texture = resourceManager->getTextureDescriptorSet(material->getTexture());
            state.setModulateColor(material->getColor());
            state.alphaTest = material->isMasked();
        }
        state.pipeline = getPipeline(topology);
        batchRenderer->draw(state, vertices, vertexCount);
    }

    void VulkanRenderer::triangle(const Vector &p1, const Vector &p2, const Vector &p3, const Color &color) {
        VulkanVertex vertices[] = {vertex(p1, color), vertex(p2, color), vertex(p3, color)};
        submitVertices(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, vertices, 3, nullptr);
    }

    void VulkanRenderer::triangle(const Vector &p1, const Vector &t1,
                                  const Vector &p2, const Vector &t2,
                                  const Vector &p3, const Vector &t3,
                                  const Material &material) {
        VulkanVertex vertices[] = {vertex(p1, t1), vertex(p2, t2), vertex(p3, t3)};
        submitVertices(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, vertices, 3, &material);
    }

    void VulkanRenderer::quad(const Vector &p1, const Vector &p2, const Vector &p3, const Vector &p4, const Color &color) {
        VulkanVertex vertices[] = {
            vertex(p1, color), vertex(p2, color), vertex(p3, color),
            vertex(p1, color), vertex(p3, color), vertex(p4, color)
        };
        submitVertices(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, vertices, 6, nullptr);
    }

    void VulkanRenderer::quad(const Vector &p1, const Vector &t1,
                              const Vector &p2, const Vector &t2,
                              const Vector &p3, const Vector &t3,
                              const Vector &p4, const Vector &t4,
                              const Material &material) {
        VulkanVertex vertices[] = {
            vertex(p1, t1), vertex(p2, t2), vertex(p3, t3),
            vertex(p1, t1), vertex(p3, t3), vertex(p4, t4)
        };
        submitVertices(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, vertices, 6, &material);
    }

    void VulkanRenderer::point(const Vector &position, Float32 size, const Color &color) {
        VulkanVertex point = vertex(position, color);
        submitVertices(VK_PRIMITIVE_TOPOLOGY_POINT_LIST, &point, 1, nullptr);
    }

    void VulkanRenderer::line(const Vector &from, const Vector &to, Float32 width, const Color &color) {
        VulkanVertex vertices[] = {vertex(from, color), vertex(to, color)};
        submitVertices(VK_PRIMITIVE_TOPOLOGY_LINE_LIST, vertices, 2, nullptr);
    }

    void VulkanRenderer::lines(const Vector positions[4], Float32 width, const Color &color) {
        VulkanVertex vertices[] = {vertex(positions[0], color), vertex(positions[1], color),
                                   vertex(positions[2], color), vertex(positions[3], color)};
        submitVertices(VK_PRIMITIVE_TOPOLOGY_LINE_STRIP, vertices, 4, nullptr);
    }

    void VulkanRenderer::lines(const std::vector<Vector> &positions, const std::vector<int>& indices,
                               const int restartIndex, Float32 width, const Color &color) {
        // One line strip per run between restart indices
        std::vector<VulkanVertex> strip;
        auto flushStrip = [&]() {
            if (strip.size() >= 2) {
                submitVertices(VK_PRIMITIVE_TOPOLOGY_LINE_STRIP, strip.data(), static_cast<uint32_t>(strip.size()), nullptr);
            }
            strip.clear();
        };
        for (int index : indices) {
            if (index == restartIndex) {
                flushStrip();
            } else if (index >= 0 && index < static_cast<int>(positions.size())) {
                strip.push_back(vertex(positions[index], color));
            }
        }
        flushStrip();
    }

    VulkanPipeline* VulkanRenderer::getPipeline(VkPrimitiveTopology topology) {
        return getPipeline(topology, depthTestEnabled, depthWriteEnabled, currentBlendFunc, wireframeEnabled,
                           stencilModeToPipelineMode(currentStencilMode));
    }

    VulkanPipeline* VulkanRenderer::getPipeline(VkPrimitiveTopology topology, bool depthTest, bool depthWrite,
                                                BlendFunc blendFunc, bool wireframe, uint32_t stencilMode) {
        uint32_t key = static_cast<uint32_t>(topology);
        key |= (depthTest ? 1u : 0u) << 8;
        key |= (depthWrite ? 1u : 0u) << 9;
        key |= static_cast<uint32_t>(blendFunc) << 10;
        key |= (wireframe ? 1u : 0u) << 12;
        key |= stencilMode << 13;
        key |= (offscreen ? 1u : 0u) << 15;

        VkRenderPass rp = renderPass->getHandle();
        VkFrontFace frontFace = offscreen ? VK_FRONT_FACE_COUNTER_CLOCKWISE : VK_FRONT_FACE_CLOCKWISE;
        return pipelineCache->getOrCreatePipeline(std::hash<uint32_t>{}(key), [&]() {
            return PipelinePresets::createPipeline(*device, rp, textureDescriptorSetLayout, topology,
                                                   depthTest, depthWrite, blendFunc, wireframe, stencilMode, frontFace);
        });
    }

    void VulkanRenderer::setProjectionMatrix(const Matrix &m) {
        projectionMatrix = m;
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

}