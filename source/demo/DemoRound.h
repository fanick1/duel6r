#ifndef DUEL6_DEMO_ROUND_H
#define DUEL6_DEMO_ROUND_H

#include <list>
#include <string>

#include "../Type.h"
#include "../Level.h"
#include "DemoFrame.h"

namespace Duel6 {
    class GameMode;

    class DemoElevator {
    public:
        bool circular;
        std::vector<Float32> controlPoints;
        DemoElevator(bool circular, const std::vector<Float32> & controlPoints);
    };


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
    public:
        Int32 width;
        Int32 height;
        std::vector<Uint16> levelData;
        std::string background;
        std::vector<DemoElevator> elevators;

    public:
        DemoLevel(const Int32 width, const Int32 height, const std::vector<Uint16> & levelData,
                 const std::string & background, const std::vector<DemoElevator> & elevators);
        std::vector<Elevator> generateElevators();
    };

    class DemoRound {
        typedef std::vector<DemoPlayerData> DemoPlayerDataList;
        typedef std::list<DemoFrame> DemoFrameList;

    private:
        bool beginning = true;
        bool ended = false;
        bool finished = false;
    public:
        DemoFrameList frames;
        DemoFrame currentFrame;
        DemoFrameList::iterator framesIterator;
        DemoPlayerDataList playerDataStart; // start of the round
        DemoPlayerDataList playerData; // playback/recording
        std::unique_ptr<DemoLevel> level;
        Uint32 frameId = 0;
        Uint32 lastFrameId = 0;

        bool hasEnded();
        void markLastFrame();
        void roundStart(bool recording, bool playing, std::vector<Player> & players);
        void nextRound(bool recording, bool playing, std::unique_ptr<Level> & level);
        DemoRound(size_t playerCount);
        void nextFrame(bool recording, bool playing);
        void nextPlayer(bool recording, bool playing, Uint32 id, Uint32 & controllerState);
        void rewind();
    };
}

#endif

