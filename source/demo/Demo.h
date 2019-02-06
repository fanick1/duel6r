#ifndef DUEL6_DEMO_H
#define DUEL6_DEMO_H

#include <list>
#include "DemoFrame.h"
#include "DemoRound.h"
#include "../File.h"
#include "../PlayerSkinColors.h"
namespace Duel6 {
    class GameMode;

    class Player;

    class DemoPlayerProfile {
    public:
        std::string name;
        PlayerSkinColors skinColors;
        DemoPlayerProfile(const std::string name, const PlayerSkinColors & skinColors);
    };

    class Demo {
        typedef std::list<DemoPlayerProfile> DemoPlayerList;
        typedef std::list<DemoRound> DemoRoundList;
    private:
        bool finished = false;
        bool roundEnd = false;
        Uint32 maxRounds; // e.g. 30
        DemoRoundList rounds;
        DemoRoundList::iterator currentRound;
        bool beginning = true;
    public:
        bool recording = true;
        bool playing = false;

        DemoPlayerList players;

        Uint32 frameId = 0;

        Uint32 initialSeed = 0;
        bool roundEnded(); // currently played level has come to an end
        void nextRound(std::unique_ptr<Level> & level);
        void roundStart(std::vector<Player> & players, const std::string & background);
        Demo(bool recording, bool playing, Uint32 maxRounds);
        void nextFrame();
        void nextPlayer(Uint32 id, Uint32 & controllerState);
        void rewind();
        bool isFinished();
        void markEndOfDemo();
        bool isBeforeStart();
        DemoLevel & getLevel();
        DemoRound & getRound();
    };
}

#endif

