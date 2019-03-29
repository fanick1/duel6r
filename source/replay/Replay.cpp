#include "Replay.h"

namespace Duel6 {
    ReplayLevel &Replay::getLevel() {
        return *(currentRound->level);
    }

    ReplayRound &Replay::getRound() {
        return *currentRound;
    }

    ReplayPlayerProfile::ReplayPlayerProfile(const std::string name, const PlayerSkinColors &skinColors)
            : name(name),
              skinColors(skinColors) {

    }

    const std::string &ReplayPlayerProfile::getName() const {
        return name;
    }

    const PlayerSkinColors &ReplayPlayerProfile::getSkinColors() const {
        return skinColors;
    }


    Replay::Replay(Uint32 maxRounds, bool globalAssistances, bool quickLiquid)
            : maxRounds(maxRounds),
              globalAssistances(globalAssistances),
              quickLiquid(quickLiquid) {
    }

    bool Replay::roundEnded() {
        return roundEnd;
    }

    void Replay::roundStart(std::vector<Player> &players, const std::string &background) {
        currentRound->roundStart(state, players, background);
    }

    void Replay::nextRound(std::unique_ptr<Level> &level) {
        if (isRecording()) {
            if (beginning) {
                initialSeed = Math::getInitialSeed();
            }
            rounds.push_back(ReplayRound(this->players.size(), initialSeed + frameId));
            if (beginning) {
                currentRound = rounds.begin();
                beginning = false;
            } else {
                currentRound->markLastFrame();
                currentRound++;
            }
            currentRound->recordLevel(level);
        }
        if (isReplaying() && !finished) {
            roundEnd = false;
            if (beginning) {
                Math::reseed(initialSeed);
                currentRound = rounds.begin();
                beginning = false;
            } else {
                currentRound++;
            }
            if (currentRound == rounds.end()) {
                finished = true;
                roundEnd = true;
            } else {
                currentRound->rewind();
            }
        }
    }

    void Replay::nextFrame() {
        if (!roundEnd) {
            frameId++;
            currentRound->nextFrame(state);
            roundEnd = currentRound->hasEnded();
        }
    }

    void Replay::updatePlayer(Uint32 id, Uint32 &controllerState) {
        if(roundEnd) {
            return;
        }

        if (isRecording()) {
            currentRound->recordPlayer(id, controllerState);
        }
        if (isReplaying()) {
            currentRound->replayPlayer(id, controllerState);
        }
    }

    void Replay::markEndOfDemo() {
        if (isRecording()) {
            currentRound->markLastFrame();
        }
    }

    void Replay::rewind() {
        currentRound = rounds.begin();
        currentRound->rewind();
        beginning = true;
        frameId = 0;
        finished = false;
        roundEnd = false;
        state = ReplayState::REPLAYING;
    }

    bool Replay::isFinished() {
        return finished;
    }

    bool Replay::isBeforeStart() {
        return beginning;
    }

    bool Replay::isRecording() {
        return state == ReplayState::RECORDING;
    }

    bool Replay::isReplaying() {
        return state == ReplayState::REPLAYING;
    }

    bool Replay::getQuickLiquid() {
        return quickLiquid;
    }

    bool Replay::getGlobalAssistances() {
        return globalAssistances;
    }
}
