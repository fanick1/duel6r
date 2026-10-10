# Vulkan renderer

Build: `cd build-vulkan && cmake .. && make -j8 duel6unity` (debug binary `duel6unityd`, validation layers on in debug builds only).
On this machine CMake needs `-DLUA_INCLUDE_DIR=/usr/include/lua5.4`.

## Shape

The game draws in immediate-mode style (`quad()`, `line()`, `font.print()` …) through the shared
`Renderer` interface. Vulkan cannot draw immediately, so the renderer records and replays:

```
Renderer call ──► VulkanRenderer::submitVertices()      builds VulkanVertex[] + DrawCommand::State
                       │                                 (pipeline, texture, mvp, viewport, scissor, stencil, time)
                       ▼
                  VulkanBatchRenderer::draw()            copies vertices into the frame's mapped vertex buffer,
                       │                                 merges with the previous draw when the state is equal
                       ▼  swapBuffers() / offscreen rerender()
                  VulkanBatchRenderer::flush()           binds pipeline / texture / dynamic state only on change,
                                                         one vkCmdDraw per command
```

* **One vertex format** (`VulkanVertex`: position, uv+layer, RGBA8 color, flags) and **one shader pair**
  (`resources/shaders/vulkan/main.vert|frag.glsl`). Untextured geometry samples a 1×1 white texture, so
  colour and textured draws share pipelines. `depth_blit.frag` is the only other shader.
* **Pipelines** are created lazily per render state (topology, depth test/write, blend, wireframe,
  stencil mode) and cached by that key (`VulkanRenderer::getPipeline`). Viewport, scissor, line width
  and stencil reference/masks are dynamic.
* **Push constants** (`PushConstants`, 88 bytes, both stages): mvp, modulate colour, alpha test, time.
  The game's projections are OpenGL style (depth −1…1); `makeDrawState()` prepends the conversion to
  Vulkan's 0…1 clip space.
  `time` drives the water wave in the vertex shader (`Vertex::Flag` 1), same formula as the GL shaders.
* **Vertex memory**: one host-visible, permanently mapped buffer per frame in flight. If a frame needs
  more, further buffers are chained and the slot is consolidated at its next `beginFrame`.
* **Textures**: `VulkanResourceManager` owns them (`TextureId` → image + descriptor set). Deletes are
  retired two frames later, when the frames that may still sample them have finished.
* **Offscreen targets** (`VulkanRendererTarget`): colour + depth/stencil images with their own render
  pass; `rerender()` runs between frames in a single-time command buffer (the game only rerenders at
  round start, on resize and on level change), `apply()` draws the colour image as a fullscreen quad
  and copies the depth with the depth-blit pipeline so dynamic objects are occluded by the static walls.
* **Y axis**: on screen, OpenGL-convention geometry is rendered with a negative viewport height
  (`VK_KHR_maintenance1`, front face `CLOCKWISE`); scissor rectangles are converted in `flush()`.
  The offscreen targets are rendered upside down instead (positive viewport, front face
  `COUNTER_CLOCKWISE`, `VulkanRenderer::setOffscreen`), exactly like an OpenGL framebuffer object,
  so pixels on shared edges are rasterised the same way as in the OpenGL renderers. Back faces are
  culled and depth compares with `LESS`, as in `glFrontFace(GL_CW)`, `glCullFace(GL_BACK)` and the
  default depth func.
* **Resize**: `clearBuffers()` compares the drawable size with the swapchain every frame and recreates
  it when they differ (Wayland never reports `VK_ERROR_OUT_OF_DATE_KHR`). Pipelines are recreated too.
* **Screenshots**: each presented frame is copied into a capture image; `makeScreenshot()` reads that.
  `Video::screenUpdate()` captures after the swap for Vulkan and before it for OpenGL.

## Files (`source/renderer/vulkan/`)

| File | Role |
|---|---|
| `VulkanRenderer` | `Renderer` implementation: state, frame lifecycle, draw methods, pipeline cache keys, swapchain recreation, screenshot capture, `VulkanRendererBuffer` (static level geometry) |
| `VulkanBatchRenderer` | `DrawCommand`, per-frame vertex buffers, merge + record |
| `VulkanRendererTarget` | offscreen render-to-texture |
| `VulkanPipeline` | pipeline builder, cache, the two presets |
| `VulkanRenderPass` | main render pass + framebuffers, command buffer manager |
| `VulkanSwapchain`, `VulkanSurface`, `VulkanDevice`, `VulkanContext` | setup |
| `VulkanResourceManager`, `VulkanTexture`, `VulkanSampler`, `VulkanDescriptor`, `VulkanMemory` | resources |
| `VulkanShader`, `VulkanTypes` | SPIR-V loading, `VulkanVertex`, enums |

## Verifying removals

Besides the render tests, two checks were used to find dead code and should be rerun after changes:
a census that parses every declaration in the Vulkan headers and counts references across `source/`
(name based), and a coverage build (`cmake -DCMAKE_CXX_FLAGS="--coverage -Wall -Wextra"`, run the
test scenarios, `gcov -f -m …/unity.cpp.gcda`) whose never-executed lines are then read one by one:
what remains unexecuted today is error handling and hardware fallbacks only.

## Testing

Builds for testing are configured with `-DD6R_AUTOMATION=ON`. `tests/render_test.py
build-vulkan/duel6unityd` runs the basic scenarios: windowed menu, windowed game and the game toggled
to fullscreen (screenshot size, frame reaching the window edges, clean exit, no validation errors).
With `--reference build-gles3/duel6unity` every screenshot, plus one at a later water-wave phase, is
compared with the OpenGL ES 3 build pixel by pixel. Results and game logs go to `.tmp/render_test`.
The automation flags are documented in `source/automation/Automation.h`. Game runs are seeded and
timed in game ticks, so two renderers screenshot the same frame.

## Other backends

The shared `Renderer` interface is unchanged. Backend changes outside the Vulkan directory:
`makeScreenshot()` of es3 and gl4 reads from the default framebuffer (the offscreen target left its
FBO bound), and `GLES3RendererTarget::resize` reallocates its textures like the GL4 target does
(before, a window grown past its start-up size showed the level only in the old area). Outside the renderers, the test automation lives in `source/automation/`
and reaches the application only through `ApplicationListener` (main loop start and update ticks)
and `Video::captureNextFrame`. gl1, gl4, es3, headless and vulkan compile; es2 fails at configure time at HEAD
too (its CMake list names a `GLES2Types.h` that does not exist).

## Performance notes

* The menu sleeps 16 ms per frame on purpose (`Menu::render`), so it shows ~59 fps at best; in-game
  rendering is capped by the 90 Hz update tick. `-print-fps` logs the frame rate once a second.
* Measured on an Intel Arc B580, Release, 1280x720 menu: the committed version spent ~10 ms per frame
  (validation layers were hard-enabled even in Release, ~6 ms without them); the current version ~1 ms.
  One menu frame is ~350 draws / 4100 vertices in both versions; the old renderer mapped and unmapped
  the vertex buffer once per batch (273 `vkUnmapMemory` per frame), the new one keeps it mapped.
* RenderDoc: run the game with `SDL_VIDEODRIVER=x11`; RenderDoc's layer has no Wayland surface support.
  Validation layers are on in `D6_DEBUG` builds only.

## Known gaps

* No bloom (GL4 only has it).
* Present mode prefers MAILBOX/IMMEDIATE; the `vsync` console command is OpenGL-only.
* `getExtensions()` returns nothing.
