//
// Created by fanick on 19.12.21.
//

#ifndef DUEL6R_SPLASH_H
#define DUEL6R_SPLASH_H
#include "Image.h"

namespace Duel6::Gui {

    class View;
    class Splash : public Image {
    private:
        Renderer &renderer;
        Font &font;
        Float32 time = 0;
        Vector textSize = {};
        Texture texture;
        Int32 maxWidth;
    public:
        Splash(RenderRoot &parent, Renderer &renderer, Font & font, const std::string & text, Int32 x, Int32 y, Int32 w);

        virtual ~Splash();

        virtual Type getType() const override {
            return Type::Splash;
        }

        virtual void update(Float32 elapsedTime) override {
            time += elapsedTime;
        }

        void setCaption(const std::string &text);

    protected:
        virtual void draw(Duel6::Renderer &renderer, const Duel6::Font &font) const override;
    private:
        void textUpdated();
    };
}


#endif //DUEL6R_SPLASH_H
