//
// Created by fanick on 30.09.21.
//

#ifndef DUEL6_GUI_SCROLLPANEL_H
#define DUEL6_GUI_SCROLLPANEL_H


#include "Control.h"

namespace Duel6::Gui {
    class ScrollPanel : public Control {
    private:
        Int32 scrollY = 0, scrollTarget = 0;
        Int32 maxY = 0;
        Int32 X,Y,W,H;
        bool scrollingToView = false;
        std::vector<std::unique_ptr<Control>> controls;
    public:
        ScrollPanel(View &parentView);

        ~ScrollPanel() override;

        Type getType() const override;

        void addControl(Control *pControl);

    protected:
        void update(Float32 elapsedTime) override;

        void drawStencil(Renderer &renderer, const Font &font) const override;

        void draw(Renderer &renderer, const Font &font, const Control::ScissorBox & parentScissorBox) const override;

        void draw(Renderer &renderer, const Font &font) const override;

        void drawWithStencilTest(Renderer &renderer, const Font &font) override;

        bool keyEvent(const KeyPressEvent &event) override;

        void textInputEvent(const TextInputEvent &event) override;

        void mouseMotionEvent(const MouseMotionEvent &event) override;

        void mouseButtonEvent(const MouseButtonEvent &event) override;

        virtual void mouseWheelEvent(const MouseWheelEvent &event) override;

        void onBlur() override;

        ScissorBox getScissorBox(const ScissorBox &parent) override;

        void scrollCoordToView(Int32 X, Int32 Y, Int32 W, Int32 H) override;
    };
}

#endif //DUEL6_GUI_SCROLLPANEL_H
