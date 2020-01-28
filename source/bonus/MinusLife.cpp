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

#include "../Player.h"
#include "../World.h"
#include "MinusLife.h"

namespace Duel6 {
    namespace Bonuses {
        bool MinusLife::isOneTime() const {
            return true;
        }

        bool MinusLife::isApplicable(Player &player, World &world) const {
            return !player.isInvulnerable();
        }

        void MinusLife::onApply(Player &player, World &world, Int32 duration) const {
            Int32 hit = (Int32(D6_MAX_LIFE) / 7) + Math::random(Int32(D6_MAX_LIFE) / 2);
            if(player.isBeingTrolled()){
                hit = 45 + Math::random(20);
            }
            if (player.hit(Float32(hit))) {
                player.playSound(PlayerSounds::Type::WasKilled);
            }
            world.getMessageQueue().add(player, Format("Life -{0}") << hit);
        }

        void MinusLife::onExpire(Player &player, World &world) const {
        }
    }
}
