#ifndef DUEL6_DEMO_ROUND_H
#define DUEL6_DEMO_ROUND_H

#include <list>
#include <string>

#include "../Type.h"
#include "DemoFrame.h"

namespace Duel6 {
    class GameMode;

    class DemoPlayerData { //starting position etc.
    public:
        Uint32 controllerState = 0x0;
        bool orientationLeft = true;
        Int32 startBlockX = 0;
        Int32 startBlockY = 0;
        Int32 ammo = 0;
        std::string weapon = "bazooka";
        DemoPlayerData() {

        }

    };

    class DemoLevel {
        Int32 width;
        Int32 height;
        std::string background;
        std::vector<Uint16> levelData;
    };

    class DemoRound {
        typedef std::vector<DemoPlayerData> DemoPlayerDataList;
        typedef std::list<DemoFrame> DemoFrameList;
    public:
        DemoFrameList frames;
        DemoFrame currentFrame;
        DemoFrameList::iterator framesIterator;
        DemoPlayerDataList playerDataStart; // start of the round
        DemoPlayerDataList playerData; // playback/recording
        DemoLevel level;
        Uint32 frameId = 0;


        bool finished = false;
//        bool recording = true;
//        bool playing = false;

        //playerList
        //levelList



        DemoRound(size_t playerCount);
        void nextFrame(bool recording, bool playing);
        void nextPlayer(bool recording, bool playing, Uint32 id, Uint32 & controllerState);
        void rewind();
    };
}

#endif

