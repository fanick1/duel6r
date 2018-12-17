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

#include "KissOfDeathShot.h"

namespace Duel6 {
    namespace {
        const Rectangle collistionRectangle = Rectangle::fromCornerAndSize(Vector(0.22f, 0.74f), Vector(0.43f, 0.22f));
        const AnimationEntry shotAnimation[] = {0, 10, 1, 10, 2, 10, 1, 10, -1, 0};
        const AnimationEntry boomAnimation[] = {0, 5, 1, 5, 0, 5, 1, 5, 0, 5, 1, 5, -1, 0};
    }

    KissOfDeathShot::KissOfDeathShot(Player &player, World &world, const LegacyWeapon &weapon, Orientation orientation)
            : LegacyShot(player, world, weapon, shotAnimation, boomAnimation, orientation, collistionRectangle) {
    }

    bool KissOfDeathShot::isColliding() const {
        return true;
    }

    bool KissOfDeathShot::hasBlood() const {
        return false;
    }

    bool KissOfDeathShot::hasPlayerExplosion() const {
        return true;
    }

    Color KissOfDeathShot::getPlayerExplosionColor() const {
        return Color::MAGENTA;
    }

    SpriteList::Iterator KissOfDeathShot::makeBoomSprite(SpriteList &spriteList) {
        auto sprite = LegacyShot::makeBoomSprite(spriteList);
        sprite->setGrow(0.01f * getPowerFactor());
        return sprite;
    }
}