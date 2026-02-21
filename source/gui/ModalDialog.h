/*
 * ModalDialog.h
 *
 *  Created on: Feb 21, 2026
 */

#ifndef DUEL6_GUI_MODALDIALOG_H_
#define DUEL6_GUI_MODALDIALOG_H_

#include <string>
#include <vector>
#include <SDL2/SDL.h>
#include "View.h"

namespace Duel6::Gui {

    class ModalDialog : public View {
    public:
        enum class Result { NONE, OK, NO, CANCEL };

        struct KeyBinding {
            SDL_Keycode key;
            Result result;
        };

        ModalDialog(Desktop &parent, Int32 screenW, Int32 screenH,
                    const std::string &message,
                    const std::vector<KeyBinding> &bindings);

        ~ModalDialog() override = default;

        bool isDismissed() const;
        Result getResult() const;
        void dismiss(Result r);

        void draw(Renderer &renderer, const Font &font, Int32 trX, Int32 trY) const override;
        bool keyEvent(const KeyPressEvent &event) override;
        void textInputEvent(const TextInputEvent &) override {}
        void mouseButtonEvent(const MouseButtonEvent &) override {}
        void mouseMotionEvent(const MouseMotionEvent &) override {}
        void mouseWheelEvent(const MouseWheelEvent &) override {}

    private:
        std::string message;
        Result result = Result::NONE;
        std::vector<KeyBinding> keyBindings;
        Int32 screenWidth, screenHeight;
        Int32 boxX, boxY, boxW, boxH;
    };

} /* namespace Duel6::Gui */

#endif /* DUEL6_GUI_MODALDIALOG_H_ */
