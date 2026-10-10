# Vulkan Rendering Codepath — Reference

Describes the renderer as of the batching rewrite (commit `f5d049b`). `handover.md` is the short
version; this document follows the code path in detail. Line numbers are approximate.

## Table of Contents
1. [Frame Lifecycle](#1-frame-lifecycle)
2. [Render Pass Architecture](#2-render-pass-architecture)
3. [Batch Renderer](#3-batch-renderer)
4. [Pipeline Management & Caching](#4-pipeline-management--caching)
5. [Vertex Format, Push Constants & Shaders](#5-vertex-format-push-constants--shaders)
6. [Offscreen Render Targets](#6-offscreen-render-targets)
7. [World Rendering Flow](#7-world-rendering-flow)
8. [Depth & Stencil State Machine](#8-depth--stencil-state-machine)
9. [Textures, Text & Resources](#9-textures-text--resources)
10. [Resize, Screenshots & Synchronisation](#10-resize-screenshots--synchronisation)
11. [Resolved Issues & Pitfalls](#11-resolved-issues--pitfalls)

---

## 1. Frame Lifecycle

### OpenGL Mental Model vs Vulkan Reality

In OpenGL, each `glDraw*()` call is immediately submitted. In this Vulkan renderer **all draw calls
are deferred**: `quad()`, `triangle()`, `line()` etc. only copy vertices into the frame's vertex
buffer and append a `DrawCommand`; the `vkCmdDraw*()` calls happen inside `flush()`.

### Frame Flow

```
Application::syncUpdateAndRender()
├── context.update()                      // game logic, once per 90 Hz tick
├── context.render()                      // only when at least one tick ran
│   ├── renderer.clearBuffers()           ← FRAME START   (VulkanRenderer.cpp:513)
│   │   ├── drawable size != swapchain extent?  → recreateSwapchain()
│   │   ├── swapchain->acquireNextImage()   (waits for this frame slot's fence)
│   │   ├── batchRenderer->beginFrame(currentFrame)
│   │   ├── commandBufferManager->beginFrame()
│   │   └── renderPass->begin()           ← main render pass starts (clears color, depth, stencil)
│   ├── ... draw calls ...                ← all deferred to the batch renderer
│   └── (control returns)
│
└── video.screenUpdate(console, font, capture)   (Video.cpp:92)
    ├── renderConsole()
    └── swapBuffers()  →  VulkanRenderer::swapBuffers()        (VulkanRenderer.cpp:555)
        ├── batchRenderer->flush(cmd, width, height, flipY=true)  ← records ALL draws
        ├── renderPass->end()
        ├── recordFrameCapture()          ← copies the swapchain image into the capture image
        ├── commandBufferManager->endFrame() + submit(wait imageAvailable, signal renderFinished, fence)
        ├── resourceManager->processDeletes()
        ├── swapchain->present()
        └── currentFrame = (currentFrame + 1) % 2
```

Two frames in flight (`MAX_FRAMES_IN_FLIGHT = 2`). The menu additionally sleeps 16 ms per frame on
purpose (`Menu::render`), so it shows ~59 fps; in-game rendering is capped by the 90 Hz update tick.

### Files
- Frame start: `VulkanRenderer::clearBuffers()` — `VulkanRenderer.cpp:513`
- Frame end: `VulkanRenderer::swapBuffers()` — `VulkanRenderer.cpp:555`
- Video swap: `Video::screenUpdate()` / `Video::swapBuffers()` — `Video.cpp:92` / `:115`

---

## 2. Render Pass Architecture

### Main Render Pass (`VulkanRenderPass.cpp:135`)

Created once at renderer init from a `RenderPassConfig`:
- **Color attachment**: swapchain format (`B8G8R8A8_UNORM` preferred, see §11), `LOAD_OP_CLEAR`,
  `finalLayout = PRESENT_SRC_KHR`
- **Depth+Stencil attachment**: `D32_SFLOAT_S8_UINT` (or the first supported depth/stencil format),
  `LOAD_OP_CLEAR`, `finalLayout = DEPTH_STENCIL_ATTACHMENT_OPTIMAL`
- **1 subpass dependency**: `EXTERNAL → 0`, stages `COLOR_ATTACHMENT_OUTPUT | EARLY_FRAGMENT_TESTS`,
  dst access `COLOR_ATTACHMENT_WRITE | DEPTH_STENCIL_ATTACHMENT_WRITE`

One framebuffer per swapchain image; `recreateFramebuffers()` rebuilds them on resize.
`VulkanCommandBufferManager` (same file) owns the command pool and one primary command buffer per
frame in flight.

### Offscreen Render Pass (`VulkanRendererTarget.cpp:61`)

Created per render target. Must be **compatible** with the main pass because every pipeline is
created against the main pass and reused in the offscreen pass:
- **Color attachment**: same format as main, `finalLayout = SHADER_READ_ONLY_OPTIMAL`
- **Depth+Stencil**: same format as main, also `finalLayout = SHADER_READ_ONLY_OPTIMAL` (the depth
  image is sampled by the depth blit, §6)
- **1 subpass dependency**: identical fields to the main pass — **critical for compatibility**

### Render Pass Compatibility (Vulkan Spec)

Two render passes are compatible if they have the same attachments (format, sample count), the same
number of subpasses, and the same subpass dependencies field for field (`srcSubpass`, `dstSubpass`,
stage masks, access masks, flags). Final layouts may differ.

---

## 3. Batch Renderer

### Overview (`VulkanBatchRenderer.cpp`)

A flat `std::vector<DrawCommand>` in submission order. Each command carries a `State` (everything
that must be equal for two draws to be issued as one), a vertex buffer handle, an offset and a
vertex count. There is no grouping by pipeline and no reordering: order is exactly the game's
draw order, which keeps stencil and overdraw semantics identical to OpenGL.

```cpp
struct DrawCommand::State {
    VulkanPipeline* pipeline;      // from the pipeline cache (§4)
    VkPrimitiveTopology topology;
    VkDescriptorSet texture;       // untextured draws use the renderer's 1x1 white texture
    Float32 modulateColor[4];      // Material color
    bool alphaTest;                // Material::isMasked()
    Float32 time;                  // water wave phase
    uint32_t stencilMode;          // 0 none, 1 write, 2 test, 3 erase
    Int32 viewport[4];             // OpenGL convention (origin bottom left, y up)
    bool scissorEnabled; Int32 scissor[4];
    Matrix mvp;                    // clipSpace * projection * view * model
};
```

### Vertex Memory

One host-visible, host-coherent, **permanently mapped** vertex buffer per frame in flight (2 MB
initially). `draw()` copies the vertices straight into it. If a frame needs more, further buffers
are chained (`allocateVertices`, `:113`); at that slot's next `beginFrame()` the chain is replaced by
one buffer large enough (`:89`), so a growth costs one extra buffer for one frame. The previous
version mapped and unmapped the buffer once per batch (273 `vkUnmapMemory` per menu frame).

### Adding Draw Commands (`draw` — `:139`)

```
VulkanRenderer::submitVertices(topology, vertices, count, material)   (VulkanRenderer.cpp:719)
├── makeDrawState(topology)         snapshot of renderer state + mvp      (:699)
├── material? → texture descriptor set, modulate color, alpha test
├── state.pipeline = getPipeline(topology)                               (:804)
└── batchRenderer->draw(state, vertices, count)
    ├── copy vertices into the frame's mapped buffer
    └── if previous command is a Draw with a list topology, contiguous in the buffer
        and an equal State → merge (vertexCount += count), else append
```

Strips (`LINE_STRIP`) never merge because two strips would be joined.

### Flush — Recording to the Command Buffer (`flush` — `:169`)

```
flush(commandBuffer, framebufferWidth, framebufferHeight, flipY)
└── for each command in order:
    ├── ClearStencil op → vkCmdClearAttachments(stencil, full framebuffer)
    ├── pipeline changed?      → vkCmdBindPipeline, vkCmdSetLineWidth(1)
    ├── viewport changed?      → vkCmdSetViewport   (negative height when flipY, VK_KHR_maintenance1)
    ├── scissor changed?       → vkCmdSetScissor    (GL rect → Vulkan rect, clamped, y flipped when flipY)
    ├── stencil mode != 0 and (pipeline changed or mode changed) → compare mask, write mask, reference
    ├── texture changed or pipeline changed → vkCmdBindDescriptorSets(set 0)
    ├── vkCmdPushConstants(mvp, modulateColor, alphaTest, time)   every draw
    ├── vertex buffer/offset changed → vkCmdBindVertexBuffers
    └── vkCmdDraw(vertexCount, 1, 0, 0)
```

`flipY = true` for the main pass (OpenGL geometry rendered right side up), `false` for offscreen
targets (stored upside down like an OpenGL FBO, §6). Viewport and scissor are dynamic in every
pipeline, so they survive pipeline changes; stencil dynamic state exists only in stencil pipelines
and is therefore re-set after a pipeline change.

A 1280x720 menu frame is ~350 draws / 4100 vertices after merging; consecutive controls alternate
between line and triangle pipelines, hence ~290 pipeline binds. GPU cost is negligible either way.

---

## 4. Pipeline Management & Caching

### State Key (`VulkanRenderer::getPipeline` — `VulkanRenderer.cpp:809`)

```
key = topology (bits 0-7)
    | depthTest (bit 8)
    | depthWrite (bit 9)
    | blendFunc (bits 10-11)       None / SrcAlpha / SrcColor
    | wireframe (bit 12)
    | stencilMode (bits 13-14)     0 none, 1 write, 2 test, 3 erase
    | offscreen (bit 15)           front face differs, see §6
```

`VulkanPipelineCache::getOrCreatePipeline(hash, creator)` (`VulkanPipeline.cpp:332`) creates a
pipeline on first use and keeps it until the cache is destroyed. The cache is recreated on swapchain
recreation (pipelines reference the render pass); pending draws are discarded first.

### Pipeline Presets (`VulkanPipeline.cpp:354`)

There are exactly two:

| Preset | Shaders | Purpose |
|---|---|---|
| `createPipeline(...)` | `main.vert` + `main.frag` | every game draw: colour, textured, lines, points |
| `createDepthBlitPipeline(...)` | `main.vert` + `depth_blit.frag` | copies an offscreen depth image into the depth buffer |

`configureCommon` sets what both share: `VulkanVertex` input, back-face culling with the given front
face, one push constant range (88 bytes, both stages), descriptor set layout 0 (one combined image
sampler), dynamic viewport, scissor and line width. `createPipeline` adds topology, depth test
(`VK_COMPARE_OP_LESS`, like the OpenGL default), depth write, wireframe, the blend function and the
stencil mode (§8). The depth blit pipeline uses depth compare `ALWAYS`, depth write on, colour write
mask 0.

Shader objects must outlive `VulkanPipelineBuilder::build()`; the builder keeps references.

### Dynamic States (set per draw, not baked into the pipeline)

- Viewport, scissor, line width — all pipelines
- Stencil reference, compare mask, write mask — stencil pipelines only

---

## 5. Vertex Format, Push Constants & Shaders

### The One Vertex Format (`VulkanTypes.h`)

```
VulkanVertex (32 bytes):
  location 0: vec3 position
  location 1: vec3 textureCoord   u, v, texture array layer
  location 2: vec4 color          RGBA8, normalised
  location 3: uint flags          Duel6::Vertex::Flag (1 = water wave)
```

Colour draws set `color` and sample the 1x1 white texture; textured draws set `color = white` and
the material colour goes into `modulateColor`. The fragment shader computes
`texture * fragColor * modulateColor`, so one shader serves both.

### Push Constants (`PushConstants`, `VulkanTypes.h`)

```
Bytes  0-63:  mat4  mvp            (vertex stage)
Bytes 64-79:  vec4  modulateColor  (fragment stage)
Bytes 80-83:  uint  alphaTest      (fragment stage; discard when alpha < 1)
Bytes 84-87:  float time           (vertex stage; water wave phase)
```

The game's projection matrices are OpenGL style (clip z in −1…1). `makeDrawState()` prepends
`openGLToVulkanClipSpace()` (z' = 0.5 z + 0.5) to the mvp; without it the near half of the frustum
is clipped.

### Shaders (`resources/shaders/vulkan/`, compiled to SPIR-V by the `vulkan_shaders` CMake target)

- `main.vert.glsl` — applies the water wave to flagged vertices (same formula as the GL shaders,
  driven by `time`), multiplies by `mvp`, passes uv+layer and colour through.
- `main.frag.glsl` — `sampler2DArray` lookup, alpha test, colour multiply.
- `depth_blit.frag.glsl` — writes `gl_FragDepth` from a sampled depth image.

Every sampled image, including the offscreen colour and depth images, has a `2D_ARRAY` view so
`sampler2DArray` fits all of them.

---

## 6. Offscreen Render Targets

### Purpose

The game pre-renders **static geometry** (background image + world blocks/sprites) into an offscreen
texture and re-renders it only when the world changes. Dynamic content (players, bonuses, elevators,
water, explosions) is rendered directly in the main pass on top.

### Two Targets (`WorldRenderer::prerender` — `WorldRenderer.cpp:769`)

1. **`target`**: background + static walls/sprites (`renderBackground()`)
2. **`explosionsTarget`**: explosion effects

### Resources (`VulkanRendererTarget::createResources` — `:127`)

Colour image (swapchain format, `COLOR_ATTACHMENT | SAMPLED`), depth/stencil image
(`DEPTH_STENCIL_ATTACHMENT | SAMPLED`), the offscreen render pass and framebuffer, a descriptor set
for the colour view (linear sampler) and one for a depth-only view (nearest sampler). Descriptor
sets are returned to the pool in `destroyResources()`; leaking them on every resize was the cause of
the old `vkUpdateDescriptorSets ... dstSet is VK_NULL_HANDLE` crash.

### Rerender (`VulkanRendererTarget::rerender` — `:364`)

Runs **between frames only**; the game rerenders at round start, on resize and on level change,
and calling it inside a frame throws.

```
rerender()
├── device.waitIdle()                   ← the frame slot's vertex buffer may still be read by the GPU
├── beginSingleTimeCommands()
├── vkCmdBeginRenderPass(offscreen pass, clear colour (0,0,0,0), depth 1, stencil 0)
├── renderer.setInFrame(true); batchRenderer.beginFrame(currentFrame)
├── renderer.setOffscreen(true); callback(); renderer.setOffscreen(false)
├── batchRenderer.flush(cmd, width, height, flipY = false)   ← rendered upside down
├── vkCmdEndRenderPass + memory barrier (attachment writes → fragment shader reads)
└── renderer.setInFrame(false); endSingleTimeCommands()      ← submits and waits for the queue
```

**Why upside down.** An OpenGL FBO stores row 0 at the bottom. Rendering the offscreen pass with a
positive viewport height stores it the same way, so the pixels on shared edges of the world blocks
are rasterised exactly as in the OpenGL renderers (this is what made the render tests reach 0 px
difference at 1000x600). The positive viewport inverts the winding, so offscreen pipelines use
`VK_FRONT_FACE_COUNTER_CLOCKWISE` (pipeline key bit 15) while the main pass uses `CLOCKWISE`, the
Vulkan equivalent of `glFrontFace(GL_CW)` after the y flip.

### Apply (`VulkanRendererTarget::apply` — `:427`)

Two fullscreen quads (6 `VulkanVertex` each, clockwise, V = 0 at the bottom row):

1. **Colour**: the offscreen colour descriptor set, `BlendFunc::SrcAlpha`, no depth test/write,
   `modulateColor` = round-start fade colour.
2. **Depth**: the depth-only descriptor set with the depth blit pipeline, which writes the sampled
   depth into the main depth buffer so that players and other dynamic objects are occluded by the
   static walls.

---

## 7. World Rendering Flow

### Full Screen Mode (`WorldRenderer::fullScreen()` — `WorldRenderer.cpp:723`)

```
fullScreen()
├── world.redrawn()? → rerender() both targets      ← between frames, before clearBuffers()
├── renderer.clearBuffers()                          ← start frame
├── target->apply(fadeColor)                         ← colour + depth of the static world
├── renderer.enableDepthTest(true), perspective camera
├── view()                                           ← dynamic content:
│   ├── elevators
│   ├── bonuses
│   ├── sprites (players, items)                     ← quadXY with material
│   ├── invulnerability rings
│   ├── water (depth write OFF, BlendFunc::SrcColor, flag 1 → vertex shader wave)
│   ├── "you are here" indicators (depth test OFF)
│   ├── player status (health bars, names)
│   └── explosions
└── game-over overlay (screenCurtain) if applicable
```

### Split Screen (`WorldRenderer::splitScreen()` — `:748`)

No offscreen target: per player, set the viewport (`setView(player.getView())`), draw the
background, then `renderStaticGeometry()` and `view()` directly. The per-draw viewport in
`DrawCommand::State` is what makes this work with deferred recording.

### Prerender — Static Content (`WorldRenderer::renderBackground()` — `:652`)

```
renderBackground()
├── orthogonal mode, background texture (fullscreen quad)
├── perspective mode + camera
├── renderStaticGeometry()
│   ├── walls   → VulkanRendererBuffer::render()    (VulkanRenderer.cpp:641)
│   └── sprites (static level sprites)
└── orthogonal mode
```

`VulkanRendererBuffer` triangulates the `FaceList` once (6 `VulkanVertex` per face) and keeps the
vertices on the CPU; `update()` only rewrites the texture layer, `render()` submits them like any
other draw, so they go through the same frame vertex buffer and merge into one `vkCmdDraw`.

### Sprite Rendering (`Sprite::render()` — `Sprite.cpp:156`)

Each sprite optionally disables the depth test (`isNoDepth()`), sets a blend func
(`isTransparent()`), builds a `Material` (texture, alpha, masked = !transparent) and calls
`renderer.quadXY(...)`, then restores state. All of this ends up as `DrawCommand::State` fields.

---

## 8. Depth & Stencil State Machine

### Depth States (pipeline key bits 8-9, compare `LESS`)

| State | depthTest | depthWrite | Used For |
|-------|-----------|------------|----------|
| Full depth | true | true | static geometry, opaque sprites |
| Test only | true | false | transparent sprites, water, overlays |
| No depth | false | false | "you are here", 2D UI, target apply |

### Stencil Modes (pipeline key bits 13-14 + dynamic state)

Used by the GUI (`gui/View.cpp`, `gui/ScrollPanel.cpp`) for controls that need clipping to a
region. The ops are baked into the pipeline, reference and masks are dynamic
(`setStencilState` in `VulkanBatchRenderer.cpp:21`).

| Mode | Value | compareOp | ops | reference | writeMask | colorWrite | Purpose |
|------|-------|-----------|-----|-----------|-----------|------------|---------|
| DISABLE | 0 | — | — | — | — | yes | normal rendering |
| WRITE | 1 | NOT_EQUAL | REPLACE | 1 | all | yes | mark pixels with 1 |
| TEST | 2 | NOT_EQUAL | KEEP | 0 | 0 | yes | draw only where marked |
| ERASE | 3 | NOT_EQUAL | REPLACE | 0 | all | **no** | clear the marks |
| CLEAR | — | — | — | — | — | — | `ClearStencil` command → `vkCmdClearAttachments` |

`Renderer::stencil(CLEAR)` does not change the mode; it appends a `ClearStencil` command at the
current position in the draw order.

---

## 9. Textures, Text & Resources

### Textures (`VulkanResourceManager`, `VulkanTexture`)

`createTexture(Image)` uploads all layers of the image through a staging buffer in a single-time
command buffer (which ends with `vkQueueWaitIdle`), creates a `2D_ARRAY` view and allocates one
descriptor set (layout: one combined image sampler) from `VulkanDescriptorManager`, whose pools are
created with `FREE_DESCRIPTOR_SET_BIT` and grow in chunks. `getTextureDescriptorSet(id)` is what
`DrawCommand::State::texture` holds.

`deleteTexture(id)` is deferred: `processDeletes()` runs once per frame in `swapBuffers()` and
destroys a texture two frames after it was deleted, when both frames in flight that could still
sample it have finished.

`VulkanMemoryManager` allocates one `VkDeviceMemory` per buffer or image (no sub-allocation); the
only buffers that exist at runtime are the per-frame vertex buffers, texture staging buffers and the
screenshot readback buffer.

### Text (`Font.cpp:80`)

Text is rendered as **textured quads**: `Font::print()` → `getTexture(text)` → `FontCache`; on a
miss `FontRenderer::renderText()` renders with SDL_ttf and uploads a one-layer array texture. The
quad is drawn with `BlendFunc::SrcAlpha` and `Material::makeColoredTexture(texture, color)`, so the
text colour arrives as `modulateColor`.

---

## 10. Resize, Screenshots & Synchronisation

### Resize

- `Application::processEvents` handles `SDL_WINDOWEVENT_SIZE_CHANGED` (fires for every size change,
  `RESIZED` only for external ones) → `Video::resize()` → resize callbacks; `WorldRenderer` resizes
  both offscreen targets and rerenders them (`WorldRenderer.cpp:41`).
- `VulkanRenderer::clearBuffers()` compares `SDL_Vulkan_GetDrawableSize` with the swapchain extent
  **every frame** and calls `recreateSwapchain()` when they differ, because Wayland never reports
  `VK_ERROR_OUT_OF_DATE_KHR`. `recreateSwapchain()` waits for the device, drops pending draws,
  recreates the swapchain, framebuffers, capture image and the pipeline cache.

### Screenshots

`swapBuffers()` copies every presented frame into a device-local capture image
(`recordFrameCapture`, `VulkanRenderer.cpp:334`). `makeScreenshot()` (`:405`) waits for the device,
copies the capture image into a host-visible buffer and converts BGRA → RGBA. `Video::screenUpdate`
therefore captures *after* the swap for Vulkan and *before* it for OpenGL, where the back buffer is
undefined once swapped.

### Synchronisation (`VulkanSwapchain.cpp:181`)

- `imageAvailableSemaphores[frame]` and `inFlightFences[frame]`: one per frame in flight.
- `renderFinishedSemaphores[image]` and `imagesInFlight[image]`: one per swapchain image (presenting
  waits on the semaphore of the image, so a semaphore is never reused while its image is queued).
- `acquireNextImage()` waits for the frame's fence, acquires, then waits for the fence of the frame
  that last used the acquired image.
- Present mode: MAILBOX if available, else IMMEDIATE, else FIFO. `minImageCount + 1` images.
- Everything outside the frame (texture uploads, offscreen rerender, screenshot readback) uses
  `beginSingleTimeCommands()` / `endSingleTimeCommands()` on the graphics queue, which submits and
  waits for the queue to go idle.

---

## 11. Resolved Issues & Pitfalls

| Symptom | Cause | Fix |
|---|---|---|
| Players drawn over walls | `apply()` wrote no depth | depth blit pipeline copies the offscreen depth (§6) |
| `vkUpdateDescriptorSets(): dstSet is VK_NULL_HANDLE` + segfault after resizing | targets leaked 2 descriptor sets per resize until the pool ran dry | sets freed in `destroyResources()`; pools grow in chunks |
| Resized window renders squashed into the top-left | swapchain never recreated (no `OUT_OF_DATE` on Wayland) | drawable size compared every frame in `clearBuffers()` |
| Whole image washed out (grey 192 → 225) | `B8G8R8A8_SRGB` swapchain re-encoded already-sRGB colours | prefer `_UNORM` formats |
| Near half of the scene clipped | OpenGL clip z (−1…1) fed to Vulkan (0…1) | `openGLToVulkanClipSpace()` prepended to mvp |
| Gun sprite blobs / wedges on block back faces in the reference diff | depth `LEQUAL`, no culling | depth `LESS`, cull back faces with `CLOCKWISE` front |
| 1-px edge lines differ from ES3 at 1000x600 | offscreen pass rendered right side up, GL FBO is upside down | offscreen pass uses positive viewport + `COUNTER_CLOCKWISE` |
| Menu labels vanish after 1-px resizes | deferred draw reordering put frames over their text | reordering removed; draws recorded in order |
| Texture deleted while a frame still samples it | immediate `vkDestroyImage` | deletes retired two frames later |
| `Menu::showMessage` rendered nothing | drew without `clearBuffers()` | replaced by `gui/ModalDialog`, which renders a normal frame |
| 40 fps menu on a fast GPU | validation layers were hard-enabled, also in Release | layers only with `D6_DEBUG`; the remaining cap is the intentional `SDL_Delay(16)` |

Practical notes:
- `clearBuffers()` is **required** before any draw call: it acquires the image, begins the command
  buffer and the render pass. Draws outside a frame are dropped.
- `VkClearRect` for the stencil clear must match the render area, not a hard-coded size.
- Validation layers are enabled in `D6_DEBUG` builds only.
- RenderDoc: run with `SDL_VIDEODRIVER=x11`; its layer has no Wayland surface support.
- Tests: `tests/render_test.py build-vulkan/duel6unityd --reference build-gles3/duel6unity` compares
  menu and in-game screenshots (resizes, fullscreen, split screen) against ES3 pixel by pixel.

---

## Appendix: File Reference

| File | Key Methods | Lines |
|------|------------|-------|
| `VulkanRenderer.cpp` | `clearBuffers()`, `swapBuffers()`, `recordFrameCapture()`, `makeScreenshot()`, `VulkanRendererBuffer`, `makeDrawState()`, `submitVertices()`, `getPipeline()` | 513, 555, 334, 405, 641, 699, 719, 809 |
| `VulkanBatchRenderer.cpp` | `setStencilState()`, `toVulkanRect()`, `beginFrame()`, `allocateVertices()`, `draw()`, `flush()` | 21, 30, 89, 113, 139, 169 |
| `VulkanRendererTarget.cpp` | `createOffscreenRenderPass()`, `createResources()`, `rerender()`, `apply()` | 61, 127, 364, 427 |
| `VulkanRenderPass.cpp` | `createRenderPass()`, `VulkanCommandBufferManager` | 135, 313 |
| `VulkanPipeline.cpp` | `VulkanPipelineBuilder::build()`, `VulkanPipelineCache`, `PipelinePresets` | 164, 332, 354 |
| `VulkanSwapchain.cpp` | `createSyncObjects()`, `acquireNextImage()`, `present()` | 181, 223, 251 |
| `VulkanTypes.h` | `VulkanVertex`, `PushConstants` | 66, 77 |
| `WorldRenderer.cpp` | `renderStaticGeometry()`, `renderBackground()`, `view()`, `fullScreen()`, `splitScreen()`, `prerender()` | 598, 652, 665, 723, 748, 769 |
| `Sprite.cpp` | `Sprite::render()` | 156 |
| `Font.cpp` | `Font::print()` | 80 |
| `Video.cpp` | `screenUpdate()`, `resize()` | 92, 190 |
