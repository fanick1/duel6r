/*
 * ModalDialog.cpp
 *
 *  Created on: Feb 21, 2026
 */

#include "ModalDialog.h"
#include "Desktop.h"

namespace Duel6::Gui {

    ModalDialog::ModalDialog(Desktop &parent, Int32 screenW, Int32 screenH,
                             const std::string &message,
                             const std::vector<KeyBinding> &bindings)
        : View(parent, 0, 0, screenW, screenH),
          message(message),
          keyBindings(bindings),
          screenWidth(screenW),
          screenHeight(screenH) {
        const Int32 fontHeight = 16;
        Font &f = getFont();
        Int32 textWidth = f.getTextWidth(message, fontHeight) + 60;
        boxW = std::max(textWidth, 200);
        boxH = fontHeight * 2 + 12;
        boxX = (screenW - boxW) / 2;
        boxY = (screenH - boxH) / 2;
    }

    bool ModalDialog::isDismissed() const {
        return result != Result::NONE;
    }

    ModalDialog::Result ModalDialog::getResult() const {
        return result;
    }

    void ModalDialog::dismiss(Result r) {
        result = r;
    }

    void ModalDialog::draw(Renderer &renderer, const Font &font, Int32 trX, Int32 trY) const {
        Matrix savedView = renderer.getViewMatrix();
        renderer.setViewMatrix(Matrix::IDENTITY);

        // Semi-transparent backdrop
        renderer.setBlendFunc(BlendFunc::SrcAlpha);
        renderer.quadXY(Vector(0, 0), Vector(screenWidth, screenHeight), Color(0, 0, 0, 128));
        renderer.setBlendFunc(BlendFunc::None);

        // Dialog box
        renderer.quadXY(Vector(boxX, boxY), Vector(boxW, boxH), Color(255, 204, 204));
        renderer.frame(Vector(boxX, boxY), Vector(boxW, boxH), 2, Color::BLACK);

        // Message text — use Int32 font.print (z=0, height=16) which goes through
        // the same code path as Label::draw, known to work with Vulkan renderer
        const Int32 fontHeight = 16;
        std::string hint = "[ESC] to cancel";
        Int32 hintWidth = font.getTextWidth(hint, fontHeight);

        font.print(boxX + 30, boxY + boxH - fontHeight - 4, Color::RED, message);
        font.print(boxX + (boxW - hintWidth) / 2, boxY + 4, Color::RED, hint);

        renderer.setViewMatrix(savedView);
    }

    bool ModalDialog::keyEvent(const KeyPressEvent &event) {
        if (event.isPressed()) {
            for (const auto &binding : keyBindings) {
                if (event.getCode() == binding.key) {
                    dismiss(binding.result);
                    return true;
                }
            }
        }
        return true; // Modal: swallow all key events
    }

} /* namespace Duel6::Gui */
