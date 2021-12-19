//
// Created by fanick on 19.12.21.
//

#ifndef DUEL6R_FONTRENDERER_H
#define DUEL6R_FONTRENDERER_H

#include <SDL2/SDL_ttf.h>
#include "Type.h"
#include "Color.h"
#include "Format.h"
#include "renderer/RendererTypes.h"
#include "renderer/Renderer.h"

namespace Duel6 {
    class FontRenderer {
    private:
        Renderer &renderer;
        TTF_Font *font;
    public:
        FontRenderer(Renderer &renderer);

        [[nodiscard]] Texture renderText(const std::string &text, bool outline) const;

        [[nodiscard]] Texture renderText(const std::string &text, bool outline, Vector &resultSize) const;

        [[nodiscard]] Texture renderTextWrapped(const std::string &text, bool outline, Uint32 maxWidth, Vector &resultSize) const;

        void setFont(TTF_Font *font);
    };
}

#endif //DUEL6R_FONTRENDERER_H
