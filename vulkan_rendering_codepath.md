# Vulkan Rendering Codepath — Comprehensive Reference

## Table of Contents
1. [Frame Lifecycle](#1-frame-lifecycle)
2. [Render Pass Architecture](#2-render-pass-architecture)
3. [Batch Renderer Pipeline](#3-batch-renderer-pipeline)
4. [Pipeline Management & Caching](#4-pipeline-management--caching)
5. [Vertex Formats & Push Constants](#5-vertex-formats--push-constants)
6. [Offscreen Render Targets](#6-offscreen-render-targets)
7. [World Rendering Flow](#7-world-rendering-flow)
8. [Depth & Stencil State Machine](#8-depth--stencil-state-machine)
9. [Text Rendering](#9-text-rendering)
10. [Known Issues & Fixes](#10-known-issues--fixes)

---

## 1. Frame Lifecycle

### OpenGL Mental Model vs Vulkan Reality

In OpenGL, each `glDraw*()` call is immediately submitted. In this Vulkan renderer, **all draw calls are deferred** into a batch renderer and only recorded to a command buffer during `flush()`.

### Frame Flow

```
Application::syncUpdateAndRender()
├── context.update()          // game logic
├── context.render()          // deferred draw calls
│   ├── renderer.clearBuffers()   ← FRAME START
│   │   ├── swapchain->acquireNextImage()
│   │   ├── batchRenderer->beginFrame()
│   │   ├── commandBufferManager->beginFrame()
│   │   └── renderPass->begin()   ← main render pass starts
│   │
│   ├── ... draw calls ...        ← all deferred to batch renderer
│   │
│   └── (control returns)
│
└── video.screenUpdate()      ← FRAME END
    ├── renderConsole()
    └── swapBuffers()
        ├── batchRenderer->flush()       ← records ALL draw commands to cmd buffer
        ├── renderPass->end()
        ├── commandBufferManager->submit()  ← GPU work submitted
        ├── batchRenderer->endFrame()
        └── swapchain->present()
```

**Key insight**: Between `clearBuffers()` and `swapBuffers()`, no Vulkan commands are actually recorded. The `quad()`, `triangle()`, `line()` etc. methods only append `DrawCommand` structs to the batch renderer's ordered list. The actual `vkCmdDraw*()` calls happen inside `flush()`.

### Files
- Frame start: `VulkanRenderer::clearBuffers()` — `VulkanRenderer.cpp:408`
- Frame end: `VulkanRenderer::swapBuffers()` — `VulkanRenderer.cpp:438`
- Video swap: `Video::swapBuffers()` — `Video.cpp:101`

---

## 2. Render Pass Architecture

### Main Render Pass (`VulkanRenderPass.cpp:~200`)

Created during renderer init. Configuration:
- **Color attachment**: Swapchain format (e.g. `B8G8R8A8_SRGB`), `LOAD_OP_CLEAR`, `finalLayout = PRESENT_SRC_KHR`
- **Depth+Stencil attachment**: `D32_SFLOAT_S8_UINT` (or similar), `LOAD_OP_CLEAR`
- **1 subpass dependency**: `EXTERNAL → 0` (waits for swapchain acquire)

### Offscreen Render Pass (`VulkanRendererTarget.cpp:60`)

Created per render target. Must be **compatible** with main pass for pipeline reuse.
- **Color attachment**: Same format as main, but `finalLayout = SHADER_READ_ONLY_OPTIMAL`
- **Depth+Stencil**: Same format as main
- **1 subpass dependency**: Same as main (EXTERNAL → 0) — **critical for compatibility**

### Render Pass Compatibility (Vulkan Spec)

Two render passes are compatible if:
1. Same number of attachments, same formats, same sample counts
2. Same number of subpasses
3. **Same number of subpass dependencies with identical fields** (srcSubpass, dstSubpass, srcStageMask, dstStageMask, srcAccessMask, dstAccessMask, dependencyFlags)

This is why both passes must have identical dependency configurations. Pipelines are always created against the **main** render pass but reused in the offscreen pass.

---

## 3. Batch Renderer Pipeline

### Overview (`VulkanBatchRenderer.cpp`)

The batch renderer collects draw commands into **ordered batches** grouped by pipeline. Order is preserved (vector, not map) for correct stencil/state sequencing.

### Adding Draw Commands

```cpp
addDrawCommand(pipeline, cmd)     // VulkanBatchRenderer.cpp:54
├── getBatch(pipeline)            // reuses last batch if same pipeline, else new batch
├── Copy vertex data to persistent storage
└── Append command to batch
```

### Flush — Recording to Command Buffer

```cpp
flush(commandBuffer, renderPass, vpWidth, vpHeight, flipY)  // :83
├── Set viewport (negative height if flipY for Y-flip)
├── Set default scissor
└── For each batch (in order):
    ├── Bind pipeline
    ├── Set line width
    └── recordDrawCommands()
```

### Recording Individual Draws (`recordDrawCommands` — `:201`)

```cpp
recordDrawCommands(commandBuffer, batch)
├── Bind vertex buffer (shared across all batches)
├── Calculate vertex stride, align vertexBufferOffset to stride
├── Map vertex buffer, copy all vertex data in one pass
├── Unmap vertex buffer
└── For each command:
    ├── Handle stencilMode=4 (clear stencil via vkCmdClearAttachments)
    ├── Set stencil dynamic state (reference, masks)
    ├── Set per-draw scissor
    ├── Push constants:
    │   ├── Vertex stage (0-63): MVP matrix
    │   └── Fragment stage (64-83): modulateColor + alphaTest  [textured only]
    ├── Bind texture descriptor set [textured only]
    └── vkCmdDraw() or vkCmdDrawIndexed()
```

### Vertex Buffer Alignment

**Critical detail**: All batches share one vertex buffer per frame. When batch N uses `TextureVertex2D` (20 bytes/vertex) and batch N+1 uses `MaterialVertex` (28 bytes/vertex), the offset must be aligned to the new stride. Without this, `firstVertex = offset / stride` produces wrong values. Fixed in `recordDrawCommands()` lines 219-232.

---

## 4. Pipeline Management & Caching

### State Hash Key (`VulkanRenderer.cpp:~1160`)

```
key = topology (bits 0-7)
    | depthTest (bit 8)
    | depthWrite (bit 9)
    | blendFunc (bits 10-11)
    | wireframe (bit 12)
    | stencilMode (bits 13-14)
    | textureFlag (bit 16)    ← differentiates color vs texture pipeline
```

### Pipeline Types

| Pipeline | Shader | Vertex Format | Descriptor Sets |
|----------|--------|---------------|-----------------|
| SimpleColor | `simple_color.{vert,frag}` | `ColorVertex` (pos+rgba) | None |
| SimpleTexture | `simple_texture.{vert,frag}` | `MaterialVertex` (pos+uv+layer+flags) | Set 0: `sampler2DArray` |
| SimpleTexture2D | `simple_texture_2d.{vert,frag}` | `TextureVertex2D` (pos+uv) | Set 0: `sampler2D` |

### Pipeline Creation Flow

```
renderer.quad(p1, t1, ..., material)
├── getTextureStatePipeline(TRIANGLE_LIST)   // VulkanRenderer.cpp:1189
│   ├── Compute hash from current state
│   └── pipelineCache->getOrCreatePipeline(hash, creator_lambda)
│       └── PipelinePresets::createSimpleTexturePipelineWithState(...)
│           └── VulkanPipelineBuilder::build()   // creates VkPipeline
└── batchRenderer->addDrawCommand(pipeline, cmd)
```

### Dynamic States (set per-draw, not baked into pipeline)

- Viewport, Scissor
- Line width
- Stencil reference, compare mask, write mask

---

## 5. Vertex Formats & Push Constants

### Vertex Formats (`VulkanTypes.h`)

```
ColorVertex (28 bytes):
  location 0: vec3 position   (12 bytes)
  location 1: vec4 color      (16 bytes)

MaterialVertex (28 bytes):
  location 0: vec3 position   (12 bytes)
  location 1: vec2 texCoord   (8 bytes)
  location 2: float texIndex  (4 bytes)  ← array layer index
  location 3: uint flags      (4 bytes)

TextureVertex2D (20 bytes):
  location 0: vec3 position   (12 bytes)
  location 1: vec2 texCoord   (8 bytes)
```

### Push Constants Layout

```
Bytes 0-63:   mat4 MVP          (vertex stage)
Bytes 64-79:  vec4 modulateColor (fragment stage)  [texture pipelines only]
Bytes 80-83:  uint alphaTest     (fragment stage)  [texture pipelines only]
```

Color pipelines only use bytes 0-63.

---

## 6. Offscreen Render Targets

### Purpose

The game pre-renders **static geometry** (background image + world blocks/sprites) into an offscreen texture. This avoids re-rendering static content every frame. Dynamic content (players, bonuses, elevators, water, explosions) is rendered directly in the main pass on top.

### Two Targets

1. **`target`**: Background + static walls/sprites (rendered in `renderBackground()`)
2. **`explosionsTarget`**: Explosion effects

### Offscreen Render Flow (`VulkanRendererTarget::rerender()` — `:329`)

```
rerender()
├── If in main frame:
│   ├── flushAndEndRenderPass()     ← end main pass
│   └── Use same command buffer
├── Begin offscreen render pass
├── Set inFrame = true
├── Execute callback (e.g. renderBackground())
│   └── All draw calls go to batch renderer as usual
├── batchRenderer->flush(flipY=true)  ← record all draws
├── vkCmdEndRenderPass()
├── Pipeline barrier (color write → shader read)
└── If was in main frame:
    └── beginMainRenderPass()       ← restart main pass
```

### Apply — Blit to Main Pass (`VulkanRendererTarget::apply()` — `:407`)

```
apply(modulateColor)
├── Build fullscreen quad (6 vertices, TextureVertex2D)
│   └── V coords flipped (0,1)→(1,0) to compensate for flipY in rerender
├── Create DrawCommand with colorDescriptorSet
├── Get/create SimpleTexture2D pipeline (sampler2D, not sampler2DArray)
└── batchRenderer->addDrawCommand()
```

**Why `sampler2D` not `sampler2DArray`?** The offscreen color image is a plain 2D image (`VK_IMAGE_VIEW_TYPE_2D`, 1 layer). The game's texture atlas uses array textures (`VK_IMAGE_VIEW_TYPE_2D_ARRAY`), but the offscreen target is not an atlas.

---

## 7. World Rendering Flow

### Full Screen Mode (`WorldRenderer::fullScreen()` — `:723`)

```
fullScreen()
├── If world redrawn: rerender() offscreen targets
├── renderer.clearBuffers()              ← start frame
├── target->apply(fadeColor)             ← blit static background
├── renderer.enableDepthTest(true)
├── Set perspective camera
├── view()                               ← render dynamic content:
│   ├── elevators
│   ├── bonuses
│   ├── sprites (players, items)         ← uses quadXY with material
│   │   ├── Non-transparent first (with depth write)
│   │   └── Transparent second (depth write OFF)
│   ├── invulnerability rings
│   ├── water (depth write OFF, BlendFunc::SrcColor)
│   ├── "you are here" indicators (depth test OFF)
│   ├── player status (health bars, names — depth write OFF)
│   └── explosions
└── Game over overlay if applicable
```

### Prerender — Static Content (`WorldRenderer::renderBackground()` — `:652`)

```
renderBackground()
├── Set orthogonal mode
├── Draw background texture (full-screen quad)
├── Set perspective mode + camera
├── renderStaticGeometry()
│   ├── walls (VulkanRendererBuffer — pre-built vertex buffer)
│   └── sprites (static level sprites, not players)
└── Set orthogonal mode
```

### Sprite Rendering (`Sprite::render()` — `Sprite.cpp:156`)

Each sprite:
1. Optionally disables depth test (`isNoDepth()`)
2. Optionally sets blend func (`isTransparent()`)
3. Gets texture index from animation frame
4. Creates `Material` with texture ID + color + alpha test flag
5. Calls `renderer.quadXY(position, size, texturePos, textureSize, material)`
6. Restores state

---

## 8. Depth & Stencil State Machine

### Depth States

| State | depthTest | depthWrite | Used For |
|-------|-----------|------------|----------|
| Full depth | true | true | Static geometry, opaque sprites |
| Test only | true | false | Transparent sprites, water, UI overlays |
| No depth | false | false | "You Are Here" indicators, 2D UI |

### Stencil Modes (used for water reflections and special effects)

| Mode | Value | compareOp | passOp | writeMask | colorWrite | Purpose |
|------|-------|-----------|--------|-----------|------------|---------|
| DISABLE | 0 | — | — | — | yes | Normal rendering |
| WRITE | 1 | NOT_EQUAL | REPLACE | 0xFF | yes | Mark stencil region |
| TEST | 2 | NOT_EQUAL | KEEP | 0x00 | yes | Draw only in marked region |
| ERASE | 3 | NOT_EQUAL | REPLACE | 0xFF | **no** | Clear stencil without color |
| CLEAR | 4 | — | — | — | — | vkCmdClearAttachments (special) |

---

## 9. Text Rendering

### Font System (`Font.cpp`)

Text is rendered as **textured quads**:
1. `Font::print()` → `getTexture(text)` → checks cache
2. Cache miss → `FontRenderer::renderText()` → SDL_ttf renders to SDL_Surface → uploaded as Vulkan texture
3. Creates `Material` with the text texture
4. Sets `BlendFunc::SrcAlpha`
5. Calls `renderer.quadXY()` with the text texture
6. Resets blend func

Text textures use the standard `sampler2DArray` pipeline (array texture with 1 layer).

---

## 10. Known Issues & Fixes

### ISSUE: Z-Ordering — Players Rendered Over World Blocks

**Symptom**: Player sprites appear on top of world geometry regardless of their Z position.

**Root cause analysis**:

The fullScreen flow is:
1. `target->apply()` — blits the pre-rendered background+walls as a 2D quad with **no depth test, no depth write** (`depthTest=false, depthWrite=false` in the apply pipeline)
2. `enableDepthTest(true)` — enables depth test for subsequent draws
3. `view()` — renders players, bonuses, etc.

The problem: `apply()` draws the background quad with depth test/write disabled, so the depth buffer is **all zeros** (cleared value = 1.0, but nothing writes to it). Then when players render with depth test enabled, they all pass (their Z < 1.0) but they don't have depth information from the walls to occlude against.

**In the OpenGL renderers**, the static geometry is either:
- Re-rendered every frame with depth (GL1), or
- The offscreen target includes depth information that gets composited

**Fix approach — Option A (Recommended): Re-render static geometry depth in the main pass**

After `target->apply()`, before `view()`, replay the static geometry with **depth write ON but color write OFF** (stencil mode 3 / ERASE could be repurposed, or create a depth-only pipeline). This populates the depth buffer with wall geometry so players get properly occluded.

```cpp
// In fullScreen(), after target->apply():
renderer.enableDepthTest(true);
renderer.enableDepthWrite(true);
// Need a way to render walls with color write disabled
// Option: add enableColorWrite(false) to renderer interface
renderStaticGeometry();  // writes depth only
renderer.enableColorWrite(true);  // (new method)
```

This requires adding `enableColorWrite(bool)` to the Renderer interface — BUT this changes the shared interface. A Vulkan-only approach: add a `depthOnlyMode` flag to the VulkanRenderer that, when set, forces `colorWriteMask = 0` on pipeline creation. The RendererBase could have a no-op default implementation.

**Fix approach — Option B: Render static geometry with depth in apply()**

Modify `apply()` to also blit depth from the offscreen target. This is more complex:
- Need a second image view for the depth attachment
- Need a shader that writes `gl_FragDepth` from a depth texture
- Vulkan doesn't support blitting depth easily

**Fix approach — Option C: Don't use offscreen target, render everything per-frame**

Simplest but defeats the purpose of the optimization.

**Recommendation**: Option A. Add a `colorWrite` state flag to VulkanRenderer (like `depthTestEnabled`), bake it into the pipeline hash. Then render static geometry with `colorWrite=false, depthWrite=true` after `apply()`. The other renderers can have a no-op or pass-through implementation.

### ISSUE: Menu::showMessage Doesn't Render

**Symptom**: `showMessage()` renders nothing — the message box and text are invisible.

**Root cause**:

`showMessage()` (`Menu.cpp:547`) does:
```cpp
renderer.quadXY(...)       // colored quad (message background)
renderer.frame(...)        // 4 lines (border)
font.print(...)            // textured quad (message text)
font.print(...)            // textured quad (cancel hint)
video.screenUpdate()       // → swapBuffers()
```

But **there is no `clearBuffers()` call**. In the Vulkan renderer, `clearBuffers()` is what acquires the swapchain image, begins the command buffer, and starts the render pass. Without it, `swapBuffers()` tries to flush and submit, but there's no active frame — the draw commands are either dropped or the renderer is not in the `inFrame` state.

In OpenGL, draw calls are immediate and the framebuffer persists between frames, so `showMessage` draws over the previous frame's content. In Vulkan, each frame is self-contained.

**Fix approach (Vulkan-only)**:

Override `showMessage` behavior in the Vulkan path by ensuring a frame is active. The cleanest approach without changing the shared Menu code:

**Option A**: Make `VulkanRenderer::swapBuffers()` tolerant of being called without `clearBuffers()`. If `!inFrame`, call `clearBuffers()` internally before `flush()`. This is a safety net in the Vulkan renderer only.

```cpp
// In VulkanRenderer::swapBuffers():
if (!inFrame) {
    clearBuffers();  // start a frame if not started
}
// ... existing flush/end/submit/present logic
```

**Option B**: Make the quad/line/print methods in VulkanRenderer auto-start a frame if not in one. This is more invasive.

**Option C (Recommended)**: Add a `clearBuffers()` call at the start of `showMessage()`. This is a one-line change in Menu.cpp that is harmless for OpenGL (just clears the screen) and necessary for Vulkan. It changes shared code but the change is safe — `clearBuffers()` is already called in every render path, and adding it to `showMessage` just ensures the frame is active:

```cpp
void Menu::showMessage(const std::string &message) {
    renderer.clearBuffers();  // ensure frame is active (needed for Vulkan)
    // ... rest unchanged
}
```

This is the simplest fix and is safe for all renderers. In OpenGL it just clears the screen before drawing the message (which is what you'd want anyway). In Vulkan it ensures the frame lifecycle is correct.

---

## Appendix: File Reference

| File | Key Methods | Lines |
|------|------------|-------|
| `VulkanRenderer.cpp` | `clearBuffers()`, `swapBuffers()`, `quad()`, `getTextureStatePipeline()` | 408, 438, 587, 1189 |
| `VulkanBatchRenderer.cpp` | `addDrawCommand()`, `flush()`, `recordDrawCommands()` | 54, 83, 201 |
| `VulkanRendererTarget.cpp` | `createOffscreenRenderPass()`, `rerender()`, `apply()` | 60, 329, 407 |
| `VulkanRenderPass.cpp` | `createRenderPass()` | ~200 |
| `VulkanPipeline.cpp` | All pipeline presets | 439+ |
| `VulkanTypes.h` | Vertex formats, state enums | 116+ |
| `WorldRenderer.cpp` | `fullScreen()`, `renderBackground()`, `view()` | 723, 652, 665 |
| `Menu.cpp` | `showMessage()`, `render()` | 547, 783 |
| `Sprite.cpp` | `Sprite::render()` | 156 |
| `Font.cpp` | `Font::print()` | 80 |
| `RendererBase.cpp` | `quadXY()`, `frame()` | 58, 115 |
