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

#include <list>
#include <unordered_map>
#include "console/Console.h"
#include "Font.h"
#include "FontException.h"
#include "Video.h"

namespace Duel6 {
    Font::Font(Renderer &renderer)
            : font(nullptr), renderer(renderer), fontRenderer(renderer),
              fontCache(renderer, 1'000), fontCacheOutlined(renderer, 1'000) {
        charHeight = 16;
    }

    Font::~Font() {
        if (font != nullptr) {
            TTF_CloseFont(font);
            font = nullptr;
        }
    }

    void Font::load(const std::string &fontFile, Console &console) {
        console.printLine(Format("...loading font: {0}") << fontFile);
        font = TTF_OpenFont(fontFile.c_str(), 32);
        if (font == nullptr) {
            D6_THROW(FontException, Format("Unable to load font {0} due to error: {1}") << fontFile << TTF_GetError());
        }
        fontRenderer.setFont(font);
    }

    Float32 Font::getTextWidth(const std::string &str, Float32 height) const {
        Int32 result;
        int origHeight;
        TTF_SizeUTF8(font, str.c_str(), &result, &origHeight);
        return result * ((Float32) height / (Float32) origHeight);
    }

    Int32 Font::getTextWidth(const std::string &str, Int32 height) const {
        Int32 result;
        int origHeight;
        TTF_SizeUTF8(font, str.c_str(), &result, &origHeight);
        return result * ((Float32) height / (Float32) origHeight);
    }

    void Font::print(Int32 x, Int32 y, const Color &color, const std::string &str, bool outline) const {
        print(Float32(x), Float32(y), 0.0f, color, str, 16, outline);
    }

    void Font::printWrapped(Int32 x, Int32 y, const Color &color, const std::string &str, Uint32 maxWidth, Vector &resultSize, bool outline) const {
        printWrapped(Float32(x), Float32(y), 0.0f, color, str, 16, maxWidth, resultSize, outline);
    }

    void Font::print(Float32 x, Float32 y, Float32 z, const Color &color, const std::string &str, Float32 height, bool outline) const {
        if (str.length() < 1) {
            return;
        }
        Texture texture = getTexture(str, outline);
        Material material = Material::makeColoredTexture(texture, color);
        Float32 width = getTextWidth(str, height);
        renderer.setBlendFunc(BlendFunc::SrcAlpha);
        renderer.quadXY(Vector(x, y, z), Vector(width, height), Vector(0, 1), Vector(1, -1), material);
        renderer.setBlendFunc(BlendFunc::None);
    }

    void Font::printWrapped(Float32 x, Float32 y, Float32 z,
                            const Color &color, const std::string &str,
                            Float32 fontHeight, Uint32 maxWidth, Vector &resultSize,
                            bool outline) const {
        if (str.length() < 1) {
            return;
        }

        Float64 scale = fontHeight / 32.0f;
        Texture texture = getTexture(str, outline, maxWidth, resultSize);
        resultSize.x *= scale;
        resultSize.y *= scale;
        Material material = Material::makeColoredTexture(texture, color);

        Float32 width = resultSize.x;
        Float32 height = resultSize.y;
        renderer.setBlendFunc(BlendFunc::SrcAlpha);
        renderer.quadXY(Vector(x, y, z), Vector(width, height), Vector(0, 1), Vector(1, -1), material);
        renderer.setBlendFunc(BlendFunc::None);
    }

    Texture Font::getTexture(const std::string &text, bool outline) const {
        const FontCache &cache = outline ? fontCacheOutlined : fontCache;
        if (cache.has(text)) {
            return cache.get(text);
        }
        Texture texture = renderText(text, outline);
        cache.add(text, texture);
        return texture;
    }

    Texture Font::getTexture(const std::string &text, bool outline, Uint32 maxWidth, Vector &resultSize) const{
        const FontCache &cache = outline ? fontCacheOutlined : fontCache;
        if (cache.has(text)) {
            return cache.get(text, resultSize.x, resultSize.y);
        }
       Texture texture = renderTextWrapped(text, outline, maxWidth, resultSize);
       cache.add(text, texture, resultSize.x, resultSize.y);
       return texture;
    }

    Texture Font::renderText(const std::string &text, bool outline) const {
        return fontRenderer.renderText(text, outline);
    }

    Texture Font::renderTextWrapped(const std::string &text, bool outline, Uint32 maxWidth, Vector &resultSize) const {
        return fontRenderer.renderTextWrapped(text, outline, maxWidth, resultSize);
    }

    const FontRenderer &Font::getFontRenderer() {
        return fontRenderer;
    }
}
