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
        return currentRound->hasEnded();
    }
    void Demo::roundStart(std::vector<Player> & players) {
        currentRound->roundStart(recording, playing, players);
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
            if(beginning){
                Math::reseed(initialSeed);
                currentRound = rounds.begin();
                beginning = false;
            } else {
                currentRound++;
            }
            if(currentRound == rounds.end()){
                finished = true;
                ended = true;
            } else {
                currentRound->nextRound(recording, playing, level);
            }
        }
    }

    void Demo::nextFrame() {
        if((playing || recording) && !ended) {
            frameId ++;
            currentRound->nextFrame(recording, playing);
            ended = roundEnded();
        }
    }

    void Demo::nextPlayer(Uint32 id, Uint32 & controllerState){
        if((playing || recording) && !ended) {
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
        ended = false;
    }

    bool Demo::isFinished(){
        return finished;
    }

    bool Demo::hasEnded(){
        return ended;
    }
}
