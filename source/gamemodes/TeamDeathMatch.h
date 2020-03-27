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

#ifndef DUEL6_GAMEMODES_TEAMDEATHMATCH_H
#define DUEL6_GAMEMODES_TEAMDEATHMATCH_H

#include "GameModeBase.h"
#include "TeamDeathMatchPlayerEventListener.h"
#include "Team.h"

namespace Duel6 {
    class TeamDeathMatch : public GameModeBase {
    private:
        Int32 teamsCount;
        bool friendlyFire;
        std::unique_ptr<PlayerEventListener> eventListener;
        TeamMap teamMap;

    public:
        TeamDeathMatch(Int32 teamsCount, bool friendlyFire)
                : teamsCount(teamsCount), friendlyFire(friendlyFire) {}

        std::string getName() const override {
            return Format("Team deathmatch ({0} teams, FF: {1})") << teamsCount << (friendlyFire ? "on" : "off");
        }

        void initializePlayersMidGame(std::vector<Player> &players) override;

        void initializePlayer(Game::PlayerDefinition &definition) override;

        void initializeRound(Game &game, std::vector<Player> &players, World &world) override;

        bool checkRoundOver(World &world, const std::vector<Player *> &alivePlayers) override;

        void initializePlayerPositions(Game &game, std::vector<Player> &players, World &world) const override;

        Ranking getRanking(const std::vector<Player> &players) const override;

        bool checkForSuddenDeathMode(World &world, const std::vector<Player *> &alivePlayers) const override;

    private:
        const Team &getPlayerTeam(Int32 playerIndex) const;
    };

}

#endif
