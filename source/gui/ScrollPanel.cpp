//
// Created by fanick on 30.09.21.
//

#include "ScrollPanel.h"
#include "View.h"

namespace Duel6::Gui {
    ScrollPanel::ScrollPanel(RenderRoot &parent) : Control(parent) {
        focusable = true;
    }

    ScrollPanel::~ScrollPanel() {

    }

    Control::Type ScrollPanel::getType() const {
        return Type::Button;
    }

    void ScrollPanel::update(Float32 elapsedTime) {
        for (auto &control: controls) {
            control->update(elapsedTime);
            if(control->y - control->getHeight() < minY) {
                minY = control->y - control->getHeight();
            } else if(control->y - height > maxY){
                maxY = control->y - height;
            }
        }
        if(scrollingToView){
            if(scrollY < Y - height ){
                scrollY = Y - height;
            }

            else if (scrollY > Y - H) {
                scrollY = Y - H;
            }

            clampScroll();
            scrollingToView = false;
        }
    }

    void ScrollPanel::drawStencil(Renderer &renderer, const Font &font) const {
        Control::drawStencil(renderer, font);
    }

    void ScrollPanel::draw(Renderer &renderer, const Font &font, const Control::ScissorBox &parentScissorBox) const {
        Matrix m = renderer.getViewMatrix();
        renderer.setViewMatrix(m * Matrix::translate(Float32(x), Float32(y - height - scrollY), 0));
        Control::ScissorBox scissorBox{parentScissorBox.rx, parentScissorBox.ry, x, y, width, height, 0, -scrollY};

        bool originalScissorTest = renderer.setScissorTest(true);
        for (auto &control: controls) {
            Control::ScissorBox s = control->getScissorBox(scissorBox);
            if(s.width <= 0 || s.height <= 0) continue;
            renderer.stencil(Duel6::Renderer::StencilMode::DISABLE);
            renderer.scissorTest(s.rx + s.x, s.ry + s.y - height - s.height + s.sy, s.width, s.height);
            control->draw(renderer, font); //TODO pass parentScissorBox
            if (control->needsStencil()) {
                renderer.stencil(Duel6::Renderer::StencilMode::WRITE);
                control->drawStencil(renderer, font);
                renderer.stencil(Duel6::Renderer::StencilMode::TEST);
                control->drawWithStencilTest(renderer, font);
                renderer.stencil(Duel6::Renderer::StencilMode::ERASE);
                control->drawStencil(renderer, font);
            }
        }
        if (!originalScissorTest) {
            renderer.setScissorTest(false);
        }

        renderer.setViewMatrix(m);
        renderer.scissorTest(parentScissorBox.rx + parentScissorBox.x, parentScissorBox.ry + parentScissorBox.y - parentScissorBox.height, parentScissorBox.width, parentScissorBox.height);
    }

    void ScrollPanel::draw(Renderer &renderer, const Font &font) const {

    }

    void ScrollPanel::drawWithStencilTest(Renderer &renderer, const Font &font) {
        Control::drawWithStencilTest(renderer, font);
    }

    bool ScrollPanel::keyEvent(const KeyPressEvent &event) {
        return Control::keyEvent(event);
    }

    void ScrollPanel::textInputEvent(const TextInputEvent &event) {
        Control::textInputEvent(event);
    }

    void ScrollPanel::mouseMotionEvent(const MouseMotionEvent &event) {
        Control::mouseMotionEvent(event);
    }

    void ScrollPanel::mouseButtonEvent(const MouseButtonEvent &event) {
//        Control::mouseButtonEvent(event);
        if(!Control::mouseIn(event, x,y,width,height)){
            return;
        }
        MouseButtonEvent translatedEvent = event.translate(-x, -y + scrollY + height);
        for (auto &control : controls) {
            control->mouseButtonEvent(translatedEvent);
        }
    }

    void ScrollPanel::mouseWheelEvent(const MouseWheelEvent &event) {
        if(!Control::mouseIn(event, x, y, width, height)){
            return;
        }

        scrollY += event.getAmountY()  * 32;
        scrollingToView = false;
        clampScroll();
    }

    void ScrollPanel::clampScroll() {
        if(scrollY > maxY){ // clamp on top
            scrollY = maxY;
        } else if (scrollY < minY){ // clamp at the bottom
            scrollY = minY;
        }
    }

    void ScrollPanel::onBlur() {
        Control::onBlur();
    }

    Control::ScissorBox ScrollPanel::getScissorBox(const Control::ScissorBox &parent) {
        return Control::getScissorBox(parent);
    }

    void ScrollPanel::addControl(Control *control) {
        control->parentControl = this;
        control->setParent(this->parent); // the parent is the owning view, not this box
        controls.push_back(std::unique_ptr<Control>(control));
        this->parent->addToFocusChain(control);
    }

    void ScrollPanel::scrollCoordToView(Int32 X, Int32 Y, Int32 W, Int32 H) {
        this->X = X;
        this->Y = Y;
        this->W = W;
        this->H = H;
        scrollingToView = true;
    }
}