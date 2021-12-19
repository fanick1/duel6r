//
// Created by fanick on 19.12.21.
//

#include "FontRenderer.h"

namespace Duel6 {

    FontRenderer::FontRenderer(Renderer &renderer):renderer(renderer) {

    }
    Texture FontRenderer::renderText(const std::string &text, bool outline) const {
        Vector resultSize;
        return renderText(text, outline, resultSize);
    }
    Texture FontRenderer::renderText(const std::string &text, bool outline, Vector &resultSize) const {
        if (outline) {
            int minX = 1000, maxX = -1000;
            int minY = 1000, maxY = -1000;

            if (text.length() == 1) {
                int minx, maxx, miny, maxy, advance;
                TTF_GlyphMetrics(font, text.front(), &minx, &maxx, &miny, &maxy, &advance);
            }
            for (const char &ch : text) {
                int minx, maxx, miny, maxy, advance;
                TTF_GlyphMetrics(font, ch, &minx, &maxx, &miny, &maxy, &advance);
                minX = minx < minX ? minx : minX;
                minY = miny < minY ? miny : minY;
                maxX = maxx > maxX ? maxx : maxX;
                maxY = maxy > maxY ? maxy : maxY;
            }
            int O = 2;
            TTF_SetFontOutline(font, O);
            SDL_Surface *outline_surface = TTF_RenderUTF8_Blended(font, text.c_str(), SDL_Color { 1, 1, 1, 255 });
            TTF_SetFontOutline(font, 0);
            SDL_Surface *surface = TTF_RenderUTF8_Blended(font, text.c_str(), SDL_Color { 255, 255, 255, 0 });
            SDL_Rect rect = { 0, 0, surface->w, surface->h };
            SDL_Rect rect2 = { O, O, outline_surface->w - 2 * O, outline_surface->h - O };
            SDL_SetSurfaceBlendMode(surface, SDL_BLENDMODE_BLEND);
            SDL_BlitScaled(surface, &rect, outline_surface, &rect2);

            SDL_FreeSurface(surface);
            Image image = Image::fromSurface(outline_surface);
            resultSize.set(image.getWidth(), image.getHeight(), image.getDepth());
            SDL_FreeSurface(outline_surface);
            return renderer.createTexture(image, TextureFilter::Linear, true);
        } else {
            SDL_Surface *surface = TTF_RenderUTF8_Blended(font, text.c_str(), SDL_Color { 255, 255, 255, 0 });
            Image image = Image::fromSurface(surface);
            resultSize.set(image.getWidth(), image.getHeight(), image.getDepth());
            SDL_FreeSurface(surface);
            return renderer.createTexture(image, TextureFilter::Linear, true);
        }
    }

    Texture FontRenderer::renderTextWrapped(const std::string &text, bool outline, Uint32 maxWidth, Vector &resultSize) const {
        if(outline){
            int O = 2;
            TTF_SetFontOutline(font, O);
            SDL_Surface *outline_surface = TTF_RenderUTF8_Blended_Wrapped(font, text.c_str(), SDL_Color{1, 1, 1, 255}, maxWidth);
            TTF_SetFontOutline(font, 0);
            SDL_Surface *surface = TTF_RenderUTF8_Blended_Wrapped(font, text.c_str(), SDL_Color{255, 255, 255, 0}, maxWidth);
            SDL_Rect rect = {0, 0 , surface->w  , surface->h};
            SDL_Rect rect2 = {O, O, outline_surface->w - 2 * O, outline_surface->h - O};

            /* blit text onto its outline */
            SDL_SetSurfaceBlendMode(surface, SDL_BLENDMODE_BLEND);
            SDL_BlitScaled(surface, &rect, outline_surface, &rect2);

            SDL_FreeSurface(surface);

            Image image = Image::fromSurface(outline_surface);
            resultSize.set(image.getWidth(), image.getHeight(), image.getDepth());

            SDL_FreeSurface(outline_surface);
            return renderer.createTexture(image, TextureFilter::Linear, true);
        } else {
            SDL_Surface *surface = TTF_RenderUTF8_Blended_Wrapped(font, text.c_str(), SDL_Color { 255, 255, 255, 0 }, maxWidth);
            Image image = Image::fromSurface(surface);
            SDL_FreeSurface(surface);
            resultSize.set(image.getWidth(), image.getHeight(), image.getDepth());
            return renderer.createTexture(image, TextureFilter::Linear, true);
        }
    }

    void FontRenderer::setFont(TTF_Font *font) {
        this->font = font;
    }
}