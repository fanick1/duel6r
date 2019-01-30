#ifndef DUEL6_DEMO_H
#define DUEL6_DEMO_H

#include "DemoRound.h"

#include <stddef.h>
#include <memory>
#include <vector>

#include "../Elevator.h"
#include "../math/Vector.h"
#include "../Orientation.h"
#include "../Player.h"
#include "../Weapon.h"

namespace Duel6 {

    DemoElevator::DemoElevator(bool circular, const std::vector<Float32> & controlPoints)
        : circular(circular),
          controlPoints(controlPoints) {

    }

    DemoLevel::DemoLevel(const Int32 width, const Int32 height,
                         const std::vector<Uint16> & levelData,
                         const std::string & background,
                         const std::vector<DemoElevator> & elevators)
        : width(width),
          height(height),
          levelData(levelData),
          background(background),
          elevators(elevators) {

    }
    std::vector<Elevator> DemoLevel::generateElevators() {
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
    DemoRound::DemoRound(size_t playerCount, Uint32 seed)
        : seed(seed),
          playerDataStart(playerCount),
          playerData(playerCount) {
        frames.push_back(currentFrame);
        framesIterator = frames.begin();

    }

    void DemoRound::rewind() {
        frameId = 0;
        framesIterator = frames.begin();
        currentFrame = *framesIterator;
        finished = false;
        beginning = true;
        ended = false;
    }

    void DemoRound::roundStart(bool recording, bool playing, std::vector<Player> & players) {
        if (recording) {
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
    bool DemoRound::hasEnded() {
        return ended;
    }
    void DemoRound::nextRound(bool recording, bool playing, std::unique_ptr<Level> & levelData) {
        if (recording) {
            std::vector<DemoElevator> elevators;
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

            level = std::make_unique<DemoLevel>(levelData->getWidth(), levelData->getHeight(),
                levelData->getLevelData(), levelData->getBackground(), elevators);
        }
        if (playing) {
            rewind();
        }
    }
    void DemoRound::markLastFrame() {
        lastFrameId = frameId;
    }
    void DemoRound::nextFrame(bool recording, bool playing) {
        Math::reseed(seed + frameId);
        if (recording) {
            if (currentFrame.controllerState.empty()) {
                currentFrame.frameId = frameId;
            } else {
                frames.push_back(currentFrame);
                currentFrame = {frameId};
            }
        }
        if(playing && !finished) { //TOODOOO
            if(currentFrame.frameId <= frameId && ++framesIterator != frames.end()) {
                currentFrame = *framesIterator;
            } else {
                if(framesIterator == frames.end()) {
                    finished = true;
                }
            }

        }

        frameId ++;
        if(frameId == lastFrameId) {
            ended = true;
        }

    }
    void DemoRound::nextPlayer(bool recording, bool playing, Uint32 id, Uint32 & controllerState) {
        if (frameId == 0) {
            if (recording) {
                playerDataStart[id].controllerState = controllerState;
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

