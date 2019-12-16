#ifndef DUEL6_RENDERER_FRAMEBUFFER_H
#define DUEL6_RENDERER_FRAMEBUFFER_H

#include "RendererTypes.h"
#include "Renderer.h"
namespace Duel6 {
    class Renderer;
    class RendererTarget {
    private:
        GLuint fbo;

        Texture texture;
        Texture depthTexture;

        GLuint width, height;
        Renderer & renderer;

    public:
        RendererTarget (GLuint depthBufferFormat, GLuint width, GLuint height, Renderer & renderer);

        virtual ~RendererTarget ();

        void use();

        void stopUse();

        void clear();

        void blit(Int32 dx, Int32 dy);

        void blitDepth();

        void render(Color & color);

        Texture getTexture();

        Texture getDepthTexture();

    };
}
#endif
