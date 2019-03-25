#include "Replay.h"
namespace Duel6 {
    ReplayLevel &Replay::getLevel() {
        return *(currentRound->level);
    }

    ReplayRound &Replay::getRound() {
        return *currentRound;
    }

    ReplayPlayerProfile::ReplayPlayerProfile(const std::string name, const PlayerSkinColors & skinColors)
        : name(name),
          skinColors(skinColors) {

    }

    Replay::Replay(bool recording, bool playing, Uint32 maxRounds, bool globalAssistances, bool quickLiquid)
        : maxRounds(maxRounds),
          recording(recording),
          playing(playing),
          globalAssistances(globalAssistances),
          quickLiquid(quickLiquid) {
    }

    bool Replay::roundEnded() {
        return roundEnd;
    }

    void Replay::roundStart(std::vector<Player> & players, const std::string & background) {
        currentRound->roundStart(recording, playing, players, background);
    }

    void Replay::nextRound(std::unique_ptr<Level> & level) {
        if(recording){
            if(beginning){
                initialSeed = Math::getInitialSeed();
            }
            rounds.push_back(ReplayRound(this->players.size(), initialSeed + frameId));
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

    void Replay::nextFrame() {
        if((playing || recording) && !roundEnd) {
            frameId ++;
            currentRound->nextFrame(recording, playing);
            roundEnd = currentRound->hasEnded();
        }
    }

    void Replay::nextPlayer(Uint32 id, Uint32 & controllerState){
        if((playing || recording) && !roundEnd) {
            currentRound->nextPlayer(recording, playing, id, controllerState);
        }
    }

    void Replay::markEndOfDemo() {
        if(recording) {
            currentRound->markLastFrame();
        }
    }

    void Replay::rewind(){
        currentRound = rounds.begin();
        currentRound->rewind();
        beginning = true;
        frameId = 0;
        finished = false;
        roundEnd = false;
    }

    bool Replay::isFinished(){
        return finished;
    }

    bool Replay::isBeforeStart() {
        return beginning;
    }

    bool Replay::getQuickLiquid() {
        return quickLiquid;
    }

    bool Replay::getGlobalAssistances() {
        return globalAssistances;
    }
}
