#ifndef DUEL6_DEMO_H
#define DUEL6_DEMO_H

#include <list>
#include "DemoRound.h"

namespace Duel6 {
    DemoRound::DemoRound(size_t playerCount)
    : playerDataStart(playerCount),
      playerData(playerCount)
    {
        frames.push_back(currentFrame);
        framesIterator = frames.begin();

    }
    void DemoRound::rewind(){
        frameId = 0;
        framesIterator = frames.begin();
        currentFrame = *framesIterator;
        finished = false;
    }
    void DemoRound::nextFrame(bool recording, bool playing) {
        if (recording) {

            if (currentFrame.controllerState.empty()) {
                currentFrame.frameId = frameId;
            } else {
                frames.push_back(currentFrame);
                currentFrame = {frameId};
            }
        }
        if(playing && !finished) { //TOODOOO
            if(currentFrame.frameId <= frameId && ++framesIterator != frames.end()){
              currentFrame = *framesIterator;
            } else {
                if(framesIterator == frames.end()){
                    finished = true;
                }

            }
        }
        frameId ++;
    }
    void DemoRound::nextPlayer(bool recording, bool playing, Uint32 id, Uint32 & controllerState){
        if(frameId == 0 ) {
            if(recording){
                playerDataStart[id].controllerState = controllerState;
                //playerDataStart[id].position = position; //TODO starting data - position, ammo, weapon, orientation
            }
            if(playing){
                playerData[id].controllerState = playerDataStart[id].controllerState;
            }
        }
        if (recording && playerData[id].controllerState != controllerState) {
            currentFrame.controllerState.push_back(id);
            currentFrame.controllerState.push_back(controllerState);
            playerData[id].controllerState = controllerState;
        }
        if (playing) {
            if(currentFrame.frameId != frameId || currentFrame.controllerState.empty()) {
                controllerState = playerData[id].controllerState;
                return;
            }
            for(size_t i = 0; i < currentFrame.controllerState.size(); i+=2) {
                Uint32 player = currentFrame.controllerState[i];
                if(player > id) {
                    controllerState = playerData[id].controllerState;
                    return;
                }
                if(player < id) {
                    controllerState = playerData[id].controllerState;
                    continue;
                }
                controllerState = currentFrame.controllerState[i + 1];
                playerData[id].controllerState = controllerState;
                return;
            }
        }
    }
}

#endif

