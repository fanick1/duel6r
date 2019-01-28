#ifndef DUEL6_DEMO_H
#define DUEL6_DEMO_H

#include <list>
#include "DemoFrame.h"
#include "DemoRound.h"
#include "../File.h"
#include "../PlayerSkinColors.h"
namespace Duel6 {
    class GameMode;


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

        Uint32 maxRounds; // e.g. 30
        DemoRoundList rounds;
        DemoRoundList::iterator currentRound;
    public:
        bool playing = false;
        bool recording = true;
        DemoPlayerList players;

        Uint32 frameId = 0;

        //playerList
        //levelList


        Demo(bool recording, bool playing, Uint32 maxRounds);
        void nextFrame();
        void nextPlayer(Uint32 id, Uint32 & controllerState);
        void rewind();
    };
}

#endif

