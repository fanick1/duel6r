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

#include "BazookaShot.h"

namespace Duel6 {
    namespace BazookaShotImpl {
        const Rectangle collistionRectangle = Rectangle::fromCornerAndSize(Vector(0.035f, 0.65f), Vector(0.59f, 0.40f));
        const AnimationEntry shotAnimation[] = {0, 164, 1, 164, -1, 0};
        const AnimationEntry boomAnimation[] = {0, 16, 2, 16, 3, 16, 4, 16, 5, 16, 6, 16, 7, 16, 8, 16, 9, 16, 10, 16,
                                                11, 16, 12, 16, 13, 16, 14, 16, 15, 16, 16, 16, 17, 16, 18, 16, 19, 16, 20, 16,
                                                21, 16, 22, 16, 23, 16, 24, 16, 25, 16, 26, 16, 27, 16, 28, 16, 29, 16, 30, 16,
                                                31, 16, 32, 16, 33, 16, 34, 16, 35, 16, -1, 0};
    }
    BazookaShot::BazookaShot(Player &owner, World &world, const LegacyWeapon &legacyWeapon, Orientation shotOrientation,
                const Weapon &weapon,
                Uint32 shotId,
                bool powerful,
                Int32 power, Float32 bulletSpeed,
                Vector &position,
                Vector &velocity)
        : LegacyShot(owner, world, legacyWeapon, BazookaShotImpl::shotAnimation, BazookaShotImpl::boomAnimation, shotOrientation, BazookaShotImpl::collistionRectangle,
            weapon, shotId, powerful, power,
            bulletSpeed, position, velocity) {
    }
    BazookaShot::BazookaShot(Player &player, World &world, const LegacyWeapon &weapon, Orientation orientation)
            : LegacyShot(player, world, weapon, BazookaShotImpl::shotAnimation, BazookaShotImpl::boomAnimation, orientation, BazookaShotImpl::collistionRectangle) {
    }

    bool BazookaShot::isColliding() const {
        return true;
    }

    bool BazookaShot::hasBlood() const {
        return false;
    }

    bool BazookaShot::hasPlayerExplosion() const {
        return true;
    }

    Color BazookaShot::getPlayerExplosionColor() const {
        return Color::RED;
    }

    Float32 BazookaShot::getExplosionRange() const {
        return 3.0f;
    }

    SpriteList::Iterator BazookaShot::makeBoomSprite(SpriteList &spriteList) {
        auto sprite = LegacyShot::makeBoomSprite(spriteList);
        sprite->setAlpha(1.0f).setBlendFunc(BlendFunc::SrcColor).setNoDepth(true);
        sprite->setGrow(1.83f * getPowerFactor());
        return sprite;
    }
}
