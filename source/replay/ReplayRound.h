#ifndef DUEL6_DEMO_ROUND_H
#define DUEL6_DEMO_ROUND_H

#include <list>
#include <string>

#include "../Type.h"
#include "../Level.h"
#include "ReplayFrame.h"

namespace Duel6 {
    class GameMode;

    class ReplayElevator {
    public:
        bool circular;
        std::vector<Float32> controlPoints;

        ReplayElevator() {}

        ReplayElevator(const ReplayElevator &e) :
                circular(e.circular),
                controlPoints(e.controlPoints) {}

        ReplayElevator(bool circular, const std::vector<Float32> &controlPoints);

        template<class Stream>
        bool serialize(Stream &s) {
            return s & circular &&
                   s & controlPoints;
        }
    };


    class ReplayPlayerData { //starting position etc.
    public:
        Uint32 controllerState = 0x0;
        bool orientationLeft = true;
        Int32 startBlockX = 0;
        Int32 startBlockY = 0;
        Int32 ammo = 0;
        std::string weapon = "bazooka";

        ReplayPlayerData() {

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

    class ReplayLevel {
    public:
        Int32 width;
        Int32 height;
        std::vector<Uint16> levelData;
        std::string background;
        std::vector<ReplayElevator> elevators;

    public:
        ReplayLevel() {}

        ReplayLevel(const ReplayLevel &l) :
                width(l.width),
                height(l.height),
                levelData(l.levelData),
                background(l.background),
                elevators(l.elevators) {

        }

        ReplayLevel(const Int32 width, const Int32 height, const std::vector<Uint16> &levelData,
                    const std::string &background, const std::vector<ReplayElevator> &elevators);

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

    class ReplayRound {
        typedef std::vector<ReplayPlayerData> ReplayPlayerDataList;
        typedef std::list<ReplayFrame> ReplayFrameList;

    private:
        bool beginning = true;
        bool ended = false;
        bool finished = false;
        Uint32 seed = 0;
    public:
        ReplayRound(size_t playerCount, Uint32 seed);

        ReplayRound() = default;

        ReplayRound(ReplayRound &&demoRound);

        ReplayRound(const ReplayRound &demoRound);

        ReplayRound &operator=(ReplayRound &&demoRound);

        ReplayRound &operator=(const ReplayRound &demoRound);

        ReplayFrameList frames;
        ReplayFrame currentFrame;
        ReplayFrameList::iterator framesIterator;
        ReplayPlayerDataList playerDataStart; // start of the round
        ReplayPlayerDataList playerData; // playback/recording
        std::unique_ptr<ReplayLevel> level;
        Uint32 frameId = 0;
        Uint32 lastFrameId = 0;

        bool hasEnded();

        void markLastFrame();

        void roundStart(bool recording, bool playing, std::vector<Player> &players, const std::string &background);

        void nextRound(bool recording, bool playing, std::unique_ptr<Level> &level);

        void nextFrame(bool recording, bool playing);

        void nextPlayer(bool recording, bool playing, Uint32 id, Uint32 &controllerState);

        void rewind();

        template<class Stream>
        bool serialize(Stream &s) {
            bool result = true;
            if (s.isDeserializer()) {
                level = std::make_unique<ReplayLevel>();
            }

            result = s & seed &&
                     s & lastFrameId &&
                     s & playerDataStart &&
                     s & *level &&
                     s & frames;

            return result;
        }

    };
}

#endif

