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
            rounds.push_back(DemoRound(this->players.size()));
            if(beginning){
                currentRound = rounds.begin();
                beginning = false;
            } else {
                currentRound->markLastFrame();
                currentRound++;
            }

            currentRound->nextRound(recording, playing, level);
        }
        if(playing){
            if(beginning){
                currentRound = rounds.begin();
                beginning = false;
            } else {
                currentRound++;
            }
            currentRound->nextRound(recording, playing, level);
        }
    }

    void Demo::nextFrame() {
        if(rounds.size() == 0){

        }
        currentRound->nextFrame(recording, playing);
    }

    void Demo::nextPlayer(Uint32 id, Uint32 & controllerState){
        currentRound->nextPlayer(recording, playing, id, controllerState);
    }

    void Demo::rewind(){
        currentRound = rounds.begin();
        currentRound->rewind();
        beginning = true;
    }
}
