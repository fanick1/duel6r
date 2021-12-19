//
// Created by fanick on 19.12.21.
//

#include "Splash.h"

namespace Duel6::Gui {
    Splash::Splash(RenderRoot &parent, Renderer &renderer, Font &font, const std::string &text, Int32 x, Int32 y, Int32 w) :
            Image(parent, Material(0, Color::BLACK, false), x, y, w, -1), //TODO Image with empty material
            renderer(renderer),
            font(font),
            texture(font.getFontRenderer().renderText(text, true, textSize)),
            maxWidth(w) {
        textUpdated();
    }

    void Splash::draw(Renderer &renderer, const Font &font) const {
        auto dx = width * (1.f + sinf(time / 2) * 0.05f) - width;
        auto dy = height * (1.f + sinf(time / 2) * 0.05f) - height;
        renderer.setBlendFunc(BlendFunc::SrcAlpha);

        Matrix rotate = Matrix::rotateAroundPoint(0.3f + sinf(time / 2) * 0.1f, Vector::UNIT_Z, Vector(x,y,0));
        renderer.setModelMatrix(rotate);
        renderer.quadXY(Vector((float) x - dx - width / 2, (float) y - dy), Vector((float) width + 2 * dx, (float) height + 2 * dy), Vector(0, 1), Vector(1, -1), material);
        renderer.setBlendFunc(BlendFunc::None);
        renderer.setModelMatrix(Matrix::IDENTITY);
    }

    Splash::~Splash() {
        renderer.freeTexture(texture);
    }

    void Splash::setCaption(const std::string &text) {
        renderer.freeTexture(texture);
        texture = font.getFontRenderer().renderText(text, true, textSize);
        textUpdated();
    }

    void Splash::textUpdated() {
        material = Material::makeColoredTexture(texture, Color::WHITE);
        auto w = std::min((Float32)maxWidth, textSize.x);
        setPosition(x, y, w, textSize.y * (w / textSize.x));
    }
}

