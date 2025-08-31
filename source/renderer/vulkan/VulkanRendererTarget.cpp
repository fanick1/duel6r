/*
* Copyright (c) 2006, Ondrej Danek (www.ondrej-danek.net)
* All rights reserved.
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright
*       notice, this list of conditions and the following disclaimer in the
*       documentation and/or other materials provided with the distribution.
*     * Neither the name of Ondrej Danek nor the
*       names of its contributors may be used to endorse or promote products
*       derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT SHALL COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
* GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
* LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
* OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "VulkanRendererTarget.h"
#include "VulkanRenderer.h"

namespace Duel6 {

    VulkanRendererTarget::VulkanRendererTarget(VulkanRenderer& renderer, ScreenParameters screenParameters)
        : renderer(renderer) {
        // For now, this is a minimal implementation that just passes through
        // In the future, this could implement off-screen rendering to textures
    }

    void VulkanRendererTarget::record(RenderCallback renderCallback) {
        // Store the callback for later execution
        callback = renderCallback;
        
        // For now, execute immediately
        if (callback) {
            callback();
        }
    }

    void VulkanRendererTarget::apply(const Color& modulateColor) {
        // For minimal implementation, do nothing to see the clear color
        // In a full implementation, this would blit the off-screen target to the main framebuffer
        // with color modulation
        
        // TODO: Uncomment to enable background rendering
        // if (callback) {
        //     callback();
        // }
    }

    void VulkanRendererTarget::resize(Int32 width, Int32 height) {
        // For minimal implementation, nothing to resize
        // In full implementation, would resize off-screen render targets
    }

    void VulkanRendererTarget::rerender() {
        // Re-execute the recorded callback
        if (callback) {
            callback();
        }
    }

} // namespace Duel6