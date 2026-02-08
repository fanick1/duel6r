# Vulkan Renderer Implementation Plan

## Architecture Analysis

The current OpenGL-style `Renderer` interface is immediate-mode oriented, which doesn't map well to Vulkan's explicit, command-buffer-based model. A hybrid approach is needed that maintains API compatibility while internally using modern rendering patterns.

## Core Architectural Changes

### 1. Command Buffer Architecture
```cpp
class VulkanCommandBuffer {
    VkCommandBuffer commandBuffer;
    VkCommandPool commandPool;
    std::vector<VulkanDrawCommand> commands;
    
    void recordDrawCommands();
    void submit(VkQueue queue);
    void reset();
};

class VulkanDrawCommand {
    enum Type { TRIANGLE, QUAD, LINE, POINT };
    Type type;
    std::vector<Vertex> vertices;
    Material material;
    Matrix mvp;
};
```

### 2. Resource Management Layer
```cpp
class VulkanResourceManager {
    VmaAllocator allocator;  // Vulkan Memory Allocator
    std::vector<VulkanBuffer> buffers;
    std::vector<VulkanTexture> textures;
    std::vector<VulkanDescriptorSet> descriptorSets;
    
    VulkanBuffer createBuffer(VkBufferUsageFlags usage, VkMemoryPropertyFlags props, size_t size);
    VulkanTexture createTexture(const Image& image, TextureFilter filter, bool clamp);
    void destroyResource(ResourceHandle handle);
};
```

### 3. Pipeline State Management
```cpp
class VulkanPipeline {
    VkPipeline pipeline;
    VkPipelineLayout layout;
    VkDescriptorSetLayout descriptorSetLayout;
    
    static VulkanPipeline createColorPipeline(VkDevice device, VkRenderPass renderPass);
    static VulkanPipeline createMaterialPipeline(VkDevice device, VkRenderPass renderPass);
};
```

## Implementation Phases

### Phase 1: Vulkan Bootstrap & Window Integration

#### 1.1 Instance & Device Setup
- Create `VulkanContext` class for Vulkan initialization
- Instance creation with validation layers (debug builds)
- Physical device selection with scoring system
- Logical device creation with required queues (graphics, present, transfer)
- Memory allocator setup using VMA (Vulkan Memory Allocator)

#### 1.2 Surface & Swapchain
- SDL2 surface creation integration (`SDL_Vulkan_CreateSurface`)
- Swapchain management with automatic recreation on resize
- Multi-buffering support (double/triple buffering)
- Present mode selection (FIFO, MAILBOX, IMMEDIATE)

#### 1.3 Files to Create/Modify:
```
source/renderer/vulkan/
├── VulkanContext.h/cpp       # Core Vulkan initialization
├── VulkanDevice.h/cpp        # Device abstraction
├── VulkanSwapchain.h/cpp     # Swapchain management
├── VulkanMemory.h/cpp        # Memory management with VMA
└── VulkanSurface.h/cpp       # SDL2 surface integration
```

### Phase 2: Resource Management

#### 2.1 Memory Management
- VMA integration for automated memory allocation
- Buffer management (vertex, index, uniform buffers)
- Staging buffer system for efficient uploads
- Memory budget tracking and garbage collection

#### 2.2 Texture System
- `VulkanTexture` class with mipmap generation
- Automatic format conversion (Image → VkFormat)
- Sampler management with caching
- Texture atlas support for batch rendering

#### 2.3 Descriptor Management
- `VulkanDescriptorPool` with automatic growth
- `VulkanDescriptorSet` wrapper for resource binding
- Descriptor set layout caching
- Dynamic uniform buffer management

### Phase 3: Shader Pipeline

#### 3.1 Shader Compilation System
- GLSL → SPIR-V compilation using `glslangValidator` or `shaderc`
- Build-time shader compilation in CMake
- Runtime shader loading from SPIR-V binaries
- Shader reflection for automatic descriptor set layout generation

#### 3.2 Pipeline State Objects
- `VulkanPipeline` class for graphics pipelines
- Pipeline caching for fast startup
- Dynamic state management (viewport, scissor)
- Specialization constants for shader variants

#### 3.3 Shader Files Structure:
```
resources/shaders/vulkan/
├── color.vert.glsl → color.vert.spv
├── color.frag.glsl → color.frag.spv  
├── material.vert.glsl → material.vert.spv
├── material.frag.glsl → material.frag.spv
└── bloom_filter.frag.glsl → bloom_filter.frag.spv
```

### Phase 4: Render Pass Architecture

#### 4.1 Render Pass Design
- Forward rendering pass for main scene
- Bloom/post-processing passes
- Multi-target rendering for effects
- Automatic render pass optimization

#### 4.2 Command Buffer Management
- Per-frame command buffer allocation
- Primary/secondary command buffer hierarchy
- Multithreaded command recording (future optimization)
- Command buffer reuse for static geometry

### Phase 5: Rendering Pipeline Implementation

#### 5.1 Batch Rendering System
```cpp
class VulkanBatchRenderer {
    struct RenderBatch {
        VulkanPipeline* pipeline;
        std::vector<DrawCommand> commands;
        VulkanDescriptorSet descriptorSet;
    };
    
    std::vector<RenderBatch> batches;
    void addDrawCommand(const DrawCommand& cmd);
    void flush(VkCommandBuffer cmdBuffer);
};
```

#### 5.2 Interface Adaptation
- Keep existing `Renderer` interface unchanged
- Internal command queuing for batch submission
- Automatic state sorting and deduplication
- Frame-based flush system

### Phase 6: Advanced Features

#### 6.1 Synchronization
- Fence-based CPU/GPU sync
- Semaphore chains for render passes
- Timeline semaphores for fine-grained sync
- Resource barriers for layout transitions

#### 6.2 Performance Optimizations
- GPU-driven rendering for large scenes
- Indirect drawing for instanced geometry
- Compute shader integration for particles/effects
- Multi-threaded command recording

## Build System Integration

### CMake Configuration
```cmake
# Add vulkan to renderer list
set(D6R_RENDERERS gl1 gl4 es2 es3 headless vulkan)

# Vulkan-specific configuration
if(D6R_RENDERER STREQUAL "vulkan")
    find_package(Vulkan REQUIRED)
    find_program(GLSLANG_VALIDATOR glslangValidator REQUIRED)
    
    # Shader compilation
    function(compile_vulkan_shaders)
        file(GLOB SHADER_SOURCES "resources/shaders/vulkan/*.glsl")
        foreach(SHADER ${SHADER_SOURCES})
            get_filename_component(SHADER_NAME ${SHADER} NAME_WE)
            set(SPIRV_OUTPUT "${CMAKE_BINARY_DIR}/shaders/${SHADER_NAME}.spv")
            add_custom_command(
                OUTPUT ${SPIRV_OUTPUT}
                COMMAND ${GLSLANG_VALIDATOR} -V ${SHADER} -o ${SPIRV_OUTPUT}
                DEPENDS ${SHADER}
            )
            list(APPEND SPIRV_SHADERS ${SPIRV_OUTPUT})
        endforeach()
        add_custom_target(vulkan_shaders ALL DEPENDS ${SPIRV_SHADERS})
    endfunction()
    
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DD6_RENDERER_VULKAN")
    target_link_libraries(duel6r Vulkan::Vulkan)
endif()
```

### Video.cpp Integration
```cpp
std::unique_ptr<Renderer> Video::createRenderer() {
#if defined(D6_RENDERER_VULKAN)
    return std::make_unique<VulkanRenderer>(window); // Pass SDL window
#elif defined(D6_RENDERER_GL4)
    return std::make_unique<GL4Renderer>();
// ... other backends
#endif
}
```

## Implementation Priority

### High Priority (Core Functionality)
1. **Basic triangle/quad rendering** - Essential for game display
2. **Texture support** - Required for sprites and UI
3. **Matrix transforms** - Camera and object positioning
4. **Basic blending** - Transparency effects

### Medium Priority (Visual Quality)
1. **Depth testing** - 3D rendering correctness  
2. **Stencil operations** - UI clipping and effects
3. **Scissor testing** - Performance optimization
4. **Wireframe mode** - Debug visualization

### Low Priority (Advanced Features)
1. **Bloom/post-processing** - Visual effects
2. **Multi-sampling** - Anti-aliasing
3. **Compute shaders** - Advanced effects
4. **Multi-threaded recording** - Performance optimization

## Migration Strategy

### Backward Compatibility
- Keep existing `Renderer` interface unchanged
- Vulkan renderer implements immediate-mode API via command queuing
- Gradual migration: OpenGL → Vulkan feature parity → OpenGL deprecation

### Testing Strategy
- Side-by-side rendering comparison (OpenGL vs Vulkan)
- Automated visual regression testing
- Performance benchmarking framework
- Memory usage profiling

### Risk Mitigation
- Implement Vulkan renderer as optional backend initially
- Maintain OpenGL renderers as fallback
- Extensive validation layer usage during development
- Cross-platform testing on multiple GPU vendors

## Dependencies

### Required Libraries
- **Vulkan SDK** - Core Vulkan headers and libraries
- **VMA (Vulkan Memory Allocator)** - Simplified memory management
- **shaderc** or **glslangValidator** - GLSL → SPIR-V compilation
- **SPIRV-Cross** (optional) - Shader reflection and optimization

### Platform Considerations
- **Windows**: Vulkan via Windows SDK or LunarG SDK
- **Linux**: System Vulkan packages or flatpak SDK
- **macOS**: MoltenVK for Metal translation layer


## Estimated Timeline

- **Phase 1** (Bootstrap): 2-3 weeks
- **Phase 2** (Resources): 2-3 weeks  
- **Phase 3** (Shaders): 1-2 weeks
- **Phase 4** (Render Passes): 1-2 weeks
- **Phase 5** (Pipeline): 3-4 weeks
- **Phase 6** (Advanced): 2-4 weeks

**Total**: ~11-18 weeks for full implementation

## Success Criteria

1. **Functional Parity**: All OpenGL renderer features working in Vulkan
2. **Performance**: ≥10% performance improvement over OpenGL on modern hardware
3. **Stability**: No crashes or validation errors in normal usage
4. **Maintainability**: Clean, documented code following Vulkan best practices
5. **Cross-platform**: Working on Windows, Linux, macOS (via MoltenVK)