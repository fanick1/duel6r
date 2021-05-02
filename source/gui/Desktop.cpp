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

#include <SDL2/SDL.h>
#include "Desktop.h"
#include "../Video.h"
#include "View.h"

namespace Duel6 {
    namespace Gui {
        namespace DesktopImpl {
            Color bcgColor(192, 192, 192);
        }

        Desktop::Desktop(Renderer &renderer, Font &font)
            : renderer(renderer),
              font(font) {
            cursorArrow = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);
            cursorIBeam = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_IBEAM);
            cursorNWSE = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENWSE);
            cursorNESW = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENESW);
            cursorWE = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEWE);
            cursorNS = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENS);

            SDL_SetCursor(cursorArrow);
        }

        Desktop::~Desktop() {
            SDL_FreeCursor(cursorArrow);
            SDL_FreeCursor(cursorIBeam);
            SDL_FreeCursor(cursorNWSE);
            SDL_FreeCursor(cursorNESW);
            SDL_FreeCursor(cursorWE);
            SDL_FreeCursor(cursorNS);
        }

        void Desktop::screenSize(Int32 scrWidth, Int32 scrHeight, Int32 trX, Int32 trY) {
            screenWidth = scrWidth;
            screenHeight = scrHeight;
            this->trX = trX;
            this->trY = trY;
        }

        void Desktop::update(Float32 elapsedTime) {
            removeClosedViews();
            for (auto &view : viewStack) {
                view->update(elapsedTime);
            }
        }

        void Desktop::draw(const Font &font) const {
            renderer.quadXY(Vector(0, 0), Vector(screenWidth, screenHeight), DesktopImpl::bcgColor);
            renderer.setViewMatrix(Matrix::translate(Float32(trX), Float32(trY), 0));

            for (auto &view : viewStack) {
                view->draw(renderer, font);
            }

            renderer.setViewMatrix(Matrix::IDENTITY);
        }

        bool Desktop::keyEvent(const KeyPressEvent &event) {
            for (auto view = viewStack.rbegin(); view != viewStack.rend(); view++) {
                if ((*view)->keyEvent(event)) return true;
            }
            return false;
        }

        void Desktop::textInputEvent(const TextInputEvent &event) {
            viewStack.back()->textInputEvent(event);
        }

        void Desktop::mouseButtonEvent(const MouseButtonEvent &event) {
            MouseButtonEvent translatedEvent = event.translate(-trX, -trY);
            viewStack.back()->mouseButtonEvent(translatedEvent);
        }

        void Desktop::mouseMotionEvent(const MouseMotionEvent &event) {
            SDL_SetCursor(cursorArrow);
            MouseMotionEvent translatedEvent = event.translate(-trX, -trY);
            viewStack.back()->mouseMotionEvent(translatedEvent);
        }

        void Desktop::mouseWheelEvent(const MouseWheelEvent &event) {
            MouseWheelEvent translatedEvent = event.translate(-trX, -trY);
            viewStack.back()->mouseWheelEvent(translatedEvent);
        }

        Font& Desktop::getFont() {
            return font;
        }

        void Desktop::addView(View *view) {
            viewStack.push_back(std::unique_ptr<View>(view));
        }

        void Desktop::closeView(View *closedView) {
            closingViews.push_back(closedView);
        }

        void Desktop::setIBeamCursor() {
            SDL_SetCursor(cursorIBeam);
        }
        void Desktop::setNWSECursor() {
            SDL_SetCursor(cursorNWSE);
        }

        void Desktop::setNESWCursor() {
            SDL_SetCursor(cursorNESW);
        }

        void Desktop::setWECursor() {
            SDL_SetCursor(cursorWE);
        }

        void Desktop::setNSCursor() {
            SDL_SetCursor(cursorNS);
        }

        void Desktop::removeClosedViews() {
            for (auto closedView = closingViews.begin(); closedView != closingViews.end(); closedView++) {
                viewStack.remove_if([&closedView](std::unique_ptr<View> &val) {
                    return val.get() == *closedView;
                });
            }
            closingViews.clear();
        }

    }
}
