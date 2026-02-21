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
        const Float32 fontHeight = 32;
        Font &f = getFont();
        Int32 textWidth = (Int32)f.getTextWidth(message, fontHeight) + 60;
        boxW = std::max(textWidth, 200);
        boxH = (Int32)(fontHeight * 2) + 4;
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

        // Message text
        const Float32 fontHeight = 32;
        font.print((Float32)boxX + 30, (Float32)boxY + fontHeight, 0.5f, Color::RED, message, fontHeight);

        // Hint text
        std::string hint = "[ESC] to cancel";
        Float32 hintWidth = font.getTextWidth(hint, fontHeight);
        font.print((Float32)boxX + ((Float32)boxW - hintWidth) / 2.0f, (Float32)boxY + 2, 0.5f, Color::RED, hint, fontHeight);

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
