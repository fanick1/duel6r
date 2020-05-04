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

#include "Tournament.h"

namespace Duel6 {
    void Tournament::initializeRound(Game &game, std::vector<Player> &players, World &world) {
        eventListener = std::make_unique<PlayerEventListener>(world.getMessageQueue(), game.getSettings());
        for (auto &player : players) {
            player.setEventListener(*eventListener);
        }
    }

    bool Tournament::checkRoundOver(World &world, const std::vector<Player *> &alivePlayers) {
        if (alivePlayers.size() == 1) {
            for (Player *player : alivePlayers) {
                world.getMessageQueue().add(*player, "You have won!");
                player->getPerson().addWins(1);
            }
            return true;
        } else if (alivePlayers.empty()) {
            for (const Player &player : world.getPlayers()) {
                world.getMessageQueue().add(player, "End of round - no winner");
            }
            return true;
        } else if(pointsLimitReached(world)){
            return true;
        }

        return false;
    }

    bool Tournament::pointsLimitReached(World & world) const {
        if (world.getGameSettings().isPointsLimit()) {
            Int32 pointsLimit = world.getGameSettings().getPointsLimit();
            for (const Player & player : world.getPlayers()) {
                if (player.getPerson().getTotalPoints() >= pointsLimit) {
                    return true;
                }
            }
        }
        return false;
    }

    bool Tournament::checkGameOver(World &world, const std::vector<Player> &players) {
        return pointsLimitReached(world);
    }
}
