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

#include "GameController.h"

namespace Duel6 {
    GameController::GameController(Instance instance) :
            open(SDL_bool::SDL_TRUE == SDL_JoystickGetAttached(instance)),
            instance(instance),
            instanceID(SDL_JoystickInstanceID(instance)),
            guid(SDL_JoystickGetGUID(instance)) {
            auto joyName = SDL_JoystickName(instance);
            name = joyName != NULL ? std::string(joyName): "<unknown>"; // unlikely to happen

    }

    const GameController::ControllerGUID &GameController::getGUID() const {
        return guid;
    }

    GameController::InstanceID GameController::getInstanceID() const {
        return instanceID;
    }

    GameController::AxisPosition GameController::getAxis(int axis) const {
        if (!open) {
            return 0;
        }
        return SDL_JoystickGetAxis(instance, axis);
    }

    const std::string &GameController::getName() const {
        return name;
    }

    bool GameController::isPressed(int button) const {
        return open && (SDL_JoystickGetButton(instance, button) == 1);
    }

    void GameController::close() {
        open = false;
        SDL_JoystickClose(instance); // This breaks everything, I don't know why
    }

    void GameController::reset(Instance instance) {
        this->instance = instance;
        open = SDL_bool::SDL_TRUE == SDL_JoystickGetAttached(instance);
        instanceID = SDL_JoystickInstanceID(instance);
    }

    bool operator==(const GameController::ControllerGUID &l, const GameController::ControllerGUID &r) {
        for (std::size_t i = 0; i < std::size(l.data); ++i) {
            if (l.data[i] != r.data[i]) {
                return false;
            }
        }
        return true;
    }
}