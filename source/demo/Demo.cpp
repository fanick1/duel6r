#include "Demo.h"
namespace Duel6 {
     DemoLevel &Demo::getLevel() {
        return *(currentRound->level);
    }
     DemoRound &Demo::getRound() {
        return *currentRound;
     }
    DemoPlayerProfile::DemoPlayerProfile(const std::string name, const PlayerSkinColors & skinColors)
        : name(name),
          skinColors(skinColors) {

    }
    Demo::Demo(bool recording, bool playing, Uint32 maxRounds)
        : maxRounds(maxRounds),
          recording(recording),
          playing(playing) {
    }

    bool Demo::roundEnded() {
        return roundEnd;
    }
    void Demo::roundStart(std::vector<Player> & players, const std::string & background) {
        currentRound->roundStart(recording, playing, players, background);
    }
    void Demo::nextRound(std::unique_ptr<Level> & level) {
        if(recording){
            if(beginning){
                initialSeed = Math::getInitialSeed();
            }
            rounds.push_back(DemoRound(this->players.size(), initialSeed + frameId));
            if(beginning){
                currentRound = rounds.begin();
                beginning = false;
            } else {
                currentRound->markLastFrame();
                currentRound++;
            }
            currentRound->nextRound(recording, playing, level);
        }
        if(playing && !finished){
            roundEnd = false;
            if(beginning){
                Math::reseed(initialSeed);
                currentRound = rounds.begin();
                beginning = false;
            } else {
                currentRound++;
            }
            if(currentRound == rounds.end()){
                finished = true;
                roundEnd = true;
            } else {
                currentRound->nextRound(recording, playing, level);
            }
        }
    }

    void Demo::nextFrame() {
        if((playing || recording) && !roundEnd) {
            frameId ++;
            currentRound->nextFrame(recording, playing);
            roundEnd = currentRound->hasEnded();
        }
    }

    void Demo::nextPlayer(Uint32 id, Uint32 & controllerState){
        if((playing || recording) && !roundEnd) {
            currentRound->nextPlayer(recording, playing, id, controllerState);
        }
    }

    void Demo::markEndOfDemo() {
        if(recording) {
            currentRound->markLastFrame();
        }
    }

    void Demo::rewind(){
        currentRound = rounds.begin();
        currentRound->rewind();
        beginning = true;
        frameId = 0;
        finished = false;
        roundEnd = false;
    }

    bool Demo::isFinished(){
        return finished;
    }

    bool Demo::isBeforeStart() {
        return beginning;
    }
}
