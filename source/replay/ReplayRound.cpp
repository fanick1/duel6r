#ifndef DUEL6_DEMO_H
#define DUEL6_DEMO_H

#include "ReplayRound.h"

#include <stddef.h>
#include <memory>
#include <vector>

#include "../Elevator.h"
#include "../math/Vector.h"
#include "../Orientation.h"
#include "../Player.h"
#include "../Weapon.h"

namespace Duel6 {

    ReplayElevator::ReplayElevator(bool circular, const std::vector<Float32> & controlPoints)
        : circular(circular),
          controlPoints(controlPoints) {

    }

    ReplayLevel::ReplayLevel(const Int32 width, const Int32 height,
                         const std::vector<Uint16> & levelData,
                         const std::string & background,
                         const std::vector<ReplayElevator> & elevators)
        : width(width),
          height(height),
          levelData(levelData),
          background(background),
          elevators(elevators) {

    }
    std::vector<Elevator> ReplayLevel::generateElevators() {
        std::vector<Elevator> result;
        result.reserve(elevators.size());
        for(const auto & demoElevator: elevators) {
            Elevator e(demoElevator.circular);
            for(size_t i = 0; i < demoElevator.controlPoints.size(); i += 3) {
                Elevator::ControlPoint c(demoElevator.controlPoints[i],
                                    demoElevator.controlPoints[i + 1],
                                    demoElevator.controlPoints[i + 2]);
                e.addControlPoint(Elevator::ControlPoint(demoElevator.controlPoints[i],
                    demoElevator.controlPoints[i + 1],
                    demoElevator.controlPoints[i + 2]));
            }
            result.push_back(e);
        }
        return result;
    }
    ReplayRound::ReplayRound(size_t playerCount, Uint32 seed)
        : seed(seed),
          playerDataStart(playerCount),
          playerData(playerCount) {
        frames.push_back(currentFrame);
        framesIterator = frames.begin();

    }
    ReplayRound::ReplayRound(Duel6::ReplayRound && demoRound):
        seed(demoRound.seed),
        frames(std::move(demoRound.frames)),
        playerDataStart(std::move(demoRound.playerDataStart)),
        playerData(playerDataStart.size()),
        level(std::move(demoRound.level)),
        lastFrameId(demoRound.lastFrameId) {
    }
    ReplayRound::ReplayRound(const Duel6::ReplayRound &demoRound):
        seed(demoRound.seed),
        frames(demoRound.frames),
        playerDataStart(demoRound.playerDataStart),
        playerData(playerDataStart.size()),
        level(std::make_unique<ReplayLevel>(*demoRound.level)),
        lastFrameId(demoRound.lastFrameId) {
    }
    ReplayRound & ReplayRound::operator = (ReplayRound && demoRound) {
        seed = demoRound.seed;
        frames = std::move(demoRound.frames);
        playerDataStart = std::move(demoRound.playerDataStart);
        playerData.resize(playerDataStart.size());
        level = std::move(demoRound.level);
        lastFrameId = demoRound.lastFrameId;
        return *this;
    }

    ReplayRound & ReplayRound::operator = (const ReplayRound & demoRound) {
        seed = demoRound.seed;
        frames = demoRound.frames;
        playerDataStart = demoRound.playerDataStart;
        playerData.resize(playerDataStart.size());
        level = std::make_unique<ReplayLevel>(*demoRound.level);
        lastFrameId = demoRound.lastFrameId;

        return *this;
    }
    void ReplayRound::rewind() {
        frameId = 0;
        framesIterator = frames.begin();
        currentFrame = *framesIterator;
        finished = false;
        beginning = true;
        ended = false;
    }

    void ReplayRound::roundStart(bool recording, bool playing, std::vector<Player> & players, const std::string & background) {
        if (recording) {
            level->background = background;
            for (size_t i = 0; i < players.size(); i++) {
                playerDataStart[i].ammo = players[i].getAmmo();
                playerDataStart[i].controllerState = players[i].getControllerStateRef();
                playerDataStart[i].orientationLeft = players[i].getOrientation() == Orientation::Left;
                auto & position = players[i].getPosition();
                playerDataStart[i].startBlockX = position.x;
                playerDataStart[i].startBlockY = position.y;
                playerDataStart[i].weapon = players[i].getWeapon().getName();
            }
        }

        if (playing) {
            for (size_t i = 0; i < players.size(); i++) {
                players[i].getControllerStateRef() = playerDataStart[i].controllerState;
                players[i].setOrientation(playerDataStart[i].orientationLeft ? Orientation::Left : Orientation::Right);
                players[i].setPosition(playerDataStart[i].startBlockX, playerDataStart[i].startBlockY);
                players[i].setWeapon(Weapon::getByName(playerDataStart[i].weapon), playerDataStart[i].ammo);
            }
        }
    }

    bool ReplayRound::hasEnded() {
        return ended;
    }
    void ReplayRound::nextRound(bool recording, bool playing, std::unique_ptr<Level> & levelData) {
        if (recording) {
            std::vector<ReplayElevator> elevators;
            elevators.reserve(levelData->getElevators().size());
            for(const auto & elevator : levelData->getElevators()){
                std::vector<Float32> controlPoints;
                controlPoints.reserve(elevator.getControlPoints().size() * 3);
                for(const auto & controlPoint : elevator.getControlPoints()) {
                    controlPoints.push_back(controlPoint.getLocation().x);
                    controlPoints.push_back(controlPoint.getLocation().y);
                    controlPoints.push_back(controlPoint.getWait());
                }
                elevators.emplace_back(elevator.isCircular(), controlPoints);
            }

            level = std::make_unique<ReplayLevel>(levelData->getWidth(), levelData->getHeight(),
                levelData->getLevelData(), levelData->getBackground(), elevators);
        }
        if (playing) {
            rewind();
        }
    }
    void ReplayRound::markLastFrame() {
        lastFrameId = frameId;
    }
    void ReplayRound::nextFrame(bool recording, bool playing) {
        frameId ++;
        if(frameId == lastFrameId) {
            ended = true;
        }

        Math::reseed(seed + frameId);
        if (recording) {
            if (currentFrame.controllerState.empty()) {
                currentFrame.frameId = frameId;
            } else {
                frames.push_back(currentFrame);
                currentFrame = {frameId};
            }
        }
        if(playing && !finished) {
            if(currentFrame.frameId < frameId && ++framesIterator != frames.end()) {
                currentFrame = *framesIterator;
            } else {
                if(framesIterator == frames.end()) {
                    finished = true; // reached last recorded frame (last input from any player), the round still continues
                }
            }

        }


    }
    void ReplayRound::nextPlayer(bool recording, bool playing, Uint32 id, Uint32 & controllerState) {
        if (frameId == 0) {
            if (recording) {
                playerDataStart[id].controllerState = controllerState;
                playerData[id].controllerState  = controllerState;
                //playerDataStart[id].position = position; //TODO starting data - position, ammo, weapon, orientation
            }
            if (playing) {
                playerData[id].controllerState = playerDataStart[id].controllerState;
            }
        }
        if (recording && playerData[id].controllerState != controllerState) {
            currentFrame.controllerState.push_back(id);
            currentFrame.controllerState.push_back(controllerState);
            playerData[id].controllerState = controllerState;
        }
        if (playing && !finished) {
            if (currentFrame.frameId != frameId || currentFrame.controllerState.empty()) {
                controllerState = playerData[id].controllerState;
                return;
            }
            for (size_t i = 0; i < currentFrame.controllerState.size(); i += 2) {
                Uint32 player = currentFrame.controllerState[i];
                if (player > id) {
                    controllerState = playerData[id].controllerState;
                    return;
                }
                if (player < id) {
                    controllerState = playerData[id].controllerState;
                    continue;
                }
                controllerState = currentFrame.controllerState[i + 1];
                playerData[id].controllerState = controllerState;
                return;
            }
        }
        if (playing && finished) {
            controllerState = playerData[id].controllerState; // to freeze players controls for the rest of the round's replay
        }
    }
}

#endif

