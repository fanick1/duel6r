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
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
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

#include "GL4RendererTarget.h"
#include "GL4Renderer.h"
#include "../../VideoException.h"

namespace Duel6 {
    GL4RendererTarget::GL4RendererTarget(GL4Renderer &renderer, ScreenParameters screenParameters)
            : width(screenParameters.getClientWidth()),
              height(screenParameters.getClientHeight()),
              renderer(renderer) {
        switch (screenParameters.getDepthBits()) {
            case 16: depthBufferFormat = GL_DEPTH_COMPONENT16; break;
            case 24: depthBufferFormat = GL_DEPTH_COMPONENT24; break;
            default: depthBufferFormat = GL_DEPTH_COMPONENT; break;
        }
        bloomTextureRatio = 2;
        glGenFramebuffers(1, &fbo);
        glGenFramebuffers(1, &glowFbo);
        glGenFramebuffers(2, bloomFbo);
        glGenTextures(1, &texture);
        glGenTextures(1, &texture2);
        glGenTextures(1, &glowTexture);
        glGenTextures(2, bloomTexture);
        glGenTextures(1, &depthTexture);

        /**
         * multiple render targets setup
         */
        for(Texture t : {texture2, glowTexture}){
            glBindTexture(GL_TEXTURE_2D_ARRAY, t);
            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);

            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA, width, height, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        }


        glBindTexture(GL_TEXTURE_2D_ARRAY, depthTexture);
        glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, depthBufferFormat, width, height, 1, 0, GL_DEPTH_COMPONENT,
                     GL_UNSIGNED_INT, nullptr);

        /**
         * first render of static geometry
         */
        {
            glBindFramebuffer(GL_FRAMEBUFFER, glowFbo);
            glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, texture2, 0, 0);
            glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, glowTexture, 0, 0);
            glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthTexture, 0, 0);
            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
                D6_THROW(VideoException, "OpenGL error: Incomplete frameBuffer ");
            }
            GLenum att[2] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
            glDrawBuffers(2, att);
            glReadBuffer(GL_COLOR_ATTACHMENT1);
            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
                D6_THROW(VideoException, "OpenGL error: Incomplete frameBuffer ");
            }
        }

        /**
         * final fbo (with static geometry)
         */
        {
            glBindFramebuffer(GL_FRAMEBUFFER, fbo);
            for(Texture t : {texture}){
                glBindTexture(GL_TEXTURE_2D_ARRAY, t);
                glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);

                glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
                glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA, width, height, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
                glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            }
            glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, texture, 0, 0);
            glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthTexture, 0, 0);
            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
                D6_THROW(VideoException, "OpenGL error: Incomplete frameBuffer ");
            }

            glDrawBuffer(GL_COLOR_ATTACHMENT0);
            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
                D6_THROW(VideoException, "OpenGL error: Incomplete frameBuffer ");
            }

        }


        /**
         * double pass bloom pipeline
         */
        for(auto i: {0,1}){
            glBindTexture(GL_TEXTURE_2D_ARRAY, bloomTexture[i]);
            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);

            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

            glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA, width / bloomTextureRatio, height / bloomTextureRatio, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            glBindFramebuffer(GL_FRAMEBUFFER, bloomFbo[i]);
            glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, bloomTexture[i], 0, 0);
            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
                D6_THROW(VideoException, "OpenGL error: Incomplete frameBuffer ");
            }
            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
                D6_THROW(VideoException, "OpenGL error: Incomplete frameBuffer ");
            }
        }


        glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    GL4RendererTarget::~GL4RendererTarget() {
        glDeleteFramebuffers(1, &fbo); //TODO delete other frame buffers?
        glDeleteTextures(1, &texture);
        glDeleteTextures(2, bloomTexture);
        glDeleteTextures(1, &texture2);
        glDeleteTextures(1, &glowTexture);
        glDeleteTextures(1, &depthTexture);
    }

    void GL4RendererTarget::record(RenderCallback renderCallback) {
        callback = renderCallback;
        rerender();
    }

    void GL4RendererTarget::apply(const Color &modulateColor) {
        if (modulateColor != Color::WHITE) {
            blitDepth();
            render(modulateColor); // render texture with color blending (slower)
        } else {
            blit(); // faster
        }
    }

    void GL4RendererTarget::bloom() {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, glowFbo);
        glReadBuffer(GL_COLOR_ATTACHMENT0);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);
        glDrawBuffer(GL_COLOR_ATTACHMENT0);
        glBlitFramebuffer(0, 0, width, height, 0, 0, width, height,  GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST);

        glReadBuffer(GL_COLOR_ATTACHMENT1);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, bloomFbo[0]);
        glDrawBuffer(GL_COLOR_ATTACHMENT0);
        glBlitFramebuffer(0, 0, width, height, 0, 0, width/bloomTextureRatio, height/bloomTextureRatio,  GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST);
        renderer.enableBloomContext();

        const int passes = 2;
        for(int i = 0 ; i < passes; i++){
            glBindFramebuffer(GL_READ_FRAMEBUFFER, bloomFbo[i % 2]);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, bloomFbo[(i + 1) % 2]);
            glReadBuffer(GL_COLOR_ATTACHMENT0);
            glDrawBuffer(GL_COLOR_ATTACHMENT0);

            // use bloomTexture from bloomFbo[i] to render into bloomFbo[i + 1]
            renderer.quadXY(Vector::ZERO, Vector((float) width/bloomTextureRatio, (float) height/bloomTextureRatio), Vector(1, 1), Vector(1, 1), Material::makeTexture(bloomTexture[i % 2]));
        }

        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);
        renderer.enableRenderContext();
        renderer.setBlendFunc(BlendFunc::SrcAlpha);

        renderer.enableDepthTest(false);
        renderer.quadXY(Vector::ZERO, Vector((float) width, (float) height), Vector(1, 1), Vector(1, 1),
                        Material::makeTexture(bloomTexture[(passes + 1) % 2]));
        renderer.enableDepthTest(true);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void GL4RendererTarget::blit() {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT,
                          GL_NEAREST);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    }

    void GL4RendererTarget::blitDepth() {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
    }

    void GL4RendererTarget::render(const Color &color) {
        renderer.quadXY(Vector::ZERO, Vector((float) width, (float) height), Vector(1, 1), Vector(1, 1),
                              Material::makeColoredTexture(texture , color));
//                              Material::makeColoredTexture(texture2 , color));
//                              Material::makeColoredTexture(glowTexture , color));
    }

    void GL4RendererTarget::resize(Int32 width, Int32 height) {
        this->width = width;
        this->height = height;
        for(Texture t : {texture, texture2, glowTexture}){
            glBindTexture(GL_TEXTURE_2D_ARRAY, t);
            glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA, width, height, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        }
        for(Texture t : { bloomTexture[0], bloomTexture[1]}){
            glBindTexture(GL_TEXTURE_2D_ARRAY, t);
            glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA, width / bloomTextureRatio, height / bloomTextureRatio, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        }
        glBindTexture(GL_TEXTURE_2D_ARRAY, depthTexture);
        glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, depthBufferFormat, width, height, 1, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, nullptr);
    }

    void GL4RendererTarget::rerender() {
        glBindFramebuffer(GL_FRAMEBUFFER, glowFbo);
        renderer.clearBuffers();
        callback();
        bloom();

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
}
