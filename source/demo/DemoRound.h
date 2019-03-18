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
        DemoElevator(){}

        DemoElevator(const DemoElevator &e) :
                circular(e.circular),
                controlPoints(e.controlPoints) {}

        DemoElevator(bool circular, const std::vector<Float32> & controlPoints);

        template<class Stream>
        bool serialize(Stream &s) {
            return s & circular &&
                   s & controlPoints;
        }
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
        template<class Stream>
        bool serialize(Stream &s) {
            return s & controllerState &&
                   s & orientationLeft &&
                   s & startBlockX &&
                   s & startBlockY &&
                   s & ammo &&
                   s & weapon;
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
        DemoLevel (){}
        DemoLevel (const DemoLevel & l):
            width(l.width),
            height(l.height),
            levelData(l.levelData),
            background(l.background),
            elevators(l.elevators) {

        }
        DemoLevel(const Int32 width, const Int32 height, const std::vector<Uint16> & levelData,
                 const std::string & background, const std::vector<DemoElevator> & elevators);
        std::vector<Elevator> generateElevators();


        template<class Stream>
        bool serialize(Stream &s) {
            return s & width &&
                   s & height &&
                   s & levelData &&
                   s & background &&
                   s & elevators;
        }
    };

    class DemoRound {
        typedef std::vector<DemoPlayerData> DemoPlayerDataList;
        typedef std::list<DemoFrame> DemoFrameList;

    private:
        bool beginning = true;
        bool ended = false;
        bool finished = false;
        Uint32 seed = 0;
    public:
        DemoFrameList frames;
        DemoFrame currentFrame;
        DemoFrameList::iterator framesIterator;
        DemoPlayerDataList playerDataStart; // start of the round
        DemoPlayerDataList playerData; // playback/recording
        std::unique_ptr<DemoLevel> level;
        Uint32 frameId = 0;
        Uint32 lastFrameId = 0;
        DemoRound() = default;
        DemoRound(DemoRound && demoRound);
        DemoRound(const DemoRound & demoRound);
        DemoRound & operator = (DemoRound && demoRound);
        DemoRound & operator = (const DemoRound & demoRound);
        bool hasEnded();
        void markLastFrame();
        void roundStart(bool recording, bool playing, std::vector<Player> & players, const std::string & background);
        void nextRound(bool recording, bool playing, std::unique_ptr<Level> & level);
        DemoRound(size_t playerCount, Uint32 seed);
        void nextFrame(bool recording, bool playing);
        void nextPlayer(bool recording, bool playing, Uint32 id, Uint32 & controllerState);
        void rewind();

        template<class Stream>
        bool serialize(Stream &s) {
            bool result = true;
            if(s.isSerializer()) {
            result = s & seed &&
                   s & lastFrameId &&
                   s & playerDataStart &&
                   s & *level &&
                   s & frames;
            } else {
                level = std::make_unique<DemoLevel>();
                result = s & seed &&
                         s & lastFrameId &&
                         s & playerDataStart;
                         s & *level &&
                         s & frames;

            }

            return result;
        }

    };
}

#endif

