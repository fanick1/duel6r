#include "Demo.h"
namespace Duel6 {
    DemoPlayerProfile::DemoPlayerProfile(const std::string name, const PlayerSkinColors & skinColors)
        : name(name),
          skinColors(skinColors) {

    }
    Demo::Demo(bool recording, bool playing, Uint32 maxRounds)
        : recording(recording),
          playing(playing),
          maxRounds(maxRounds)
          {

    }

    void Demo::nextFrame() {
        if(rounds.size() == 0){
            rounds.push_back(DemoRound(players.size()));
            currentRound = rounds.begin();
        }
        currentRound->nextFrame(recording, playing);
    }

    void Demo::nextPlayer(Uint32 id, Uint32 & controllerState){
        if(rounds.size() == 0){
            rounds.push_back(DemoRound(players.size()));
            currentRound = rounds.begin();
        }
        currentRound->nextPlayer(recording, playing, id, controllerState);
    }

    void Demo::rewind(){
        currentRound = rounds.begin();
        currentRound->rewind();
    }
}
