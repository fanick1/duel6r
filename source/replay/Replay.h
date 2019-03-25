#ifndef DUEL6_DEMO_H
#define DUEL6_DEMO_H

#include <list>
#include "ReplayFrame.h"
#include "ReplayRound.h"
#include "../File.h"
#include "../PlayerSkinColors.h"

namespace Duel6 {
    class GameMode;

    class Player;

    class ReplayPlayerProfile {
    public:
        std::string name;
        PlayerSkinColors skinColors;

        ReplayPlayerProfile() {}

        ReplayPlayerProfile(const ReplayPlayerProfile &p) :
                name(p.name),
                skinColors(p.skinColors) {}

        ReplayPlayerProfile(const std::string name, const PlayerSkinColors &skinColors);

        template<class Stream>
        bool serialize(Stream &s) {
            return s & name &&
                   s & skinColors;
        }

    };

    class Replay {
        typedef std::list<ReplayPlayerProfile> ReplayPlayerList;
        typedef std::list<ReplayRound> ReplayRoundList;
    private:
        bool finished = false;
        bool roundEnd = false;
        Uint32 maxRounds; // e.g. 30
        ReplayRoundList rounds;
        ReplayRoundList::iterator currentRound;
        bool beginning = true;
        bool globalAssistances = false;
        bool quickLiquid = false;
    public:
        bool recording = true;
        bool playing = false;

        Replay(){};
        Replay(bool recording, bool playing, Uint32 maxRounds, bool globalAssistances, bool quickLiquid);

        ReplayPlayerList players;

        Uint32 frameId = 0;

        Uint32 initialSeed = 0;

        bool getQuickLiquid();

        bool getGlobalAssistances();

        bool roundEnded(); // currently played level has come to an end
        void nextRound(std::unique_ptr<Level> &level);

        void roundStart(std::vector<Player> &players, const std::string &background);

        void nextFrame();

        void nextPlayer(Uint32 id, Uint32 &controllerState);

        void rewind();

        bool isFinished();

        void markEndOfDemo();

        bool isBeforeStart();

        ReplayLevel &getLevel();

        ReplayRound &getRound();

        template<class Stream>
        bool serialize(Stream &s) {
            return s & maxRounds &&
                   s & globalAssistances &&
                   s & quickLiquid &&
                   s & initialSeed &&
                   s & players &&
                   s & rounds;
        }
    };
}

template<typename Stream>
bool serialize(Stream &s, Duel6::Color &c) {
    bool result = true;

    if (s.isDeserializer()) {
        Uint8 r, g, b, a;
        result = s & r &&
                 s & g &&
                 s & b &&
                 s & a;
        c.set(r, g, b, a);
    } else {
        result = s & c.getRed() &&
                 s & c.getGreen() &&
                 s & c.getBlue() &&
                 s & c.getAlpha();
    }

    return result;

}

template<typename Stream>
bool serialize(Stream &s, Duel6::PlayerSkinColors::Hair &hair) {
    bool result = true;

    if (s.isDeserializer()) {
        Uint8 h;
        result = s.safe_max(h, (Uint8)2); // 0,1,2
        if (result) {
            hair = static_cast<Duel6::PlayerSkinColors::Hair>(h);
        }
    } else {
        result = s & static_cast<Uint8>(hair);
    }

    return result;

}

template<typename Stream>
bool serialize(Stream &s, Duel6::PlayerSkinColors &c) {
    bool result = true;
    if (s.isDeserializer()) {
        Duel6::Color colors[11];
        Duel6::PlayerSkinColors::Hair h;
        bool headBand;
        for (size_t i = 0; i < 11 && result; ++i) {
            result &= s & colors[i];
        }
        result &= s & h &&
                  s & headBand;
        if (result) {
            for (size_t i = 0; i < 11 && result; ++i) {
                c.set(static_cast<Duel6::PlayerSkinColors::BodyPart>(i), colors[i]);
            }
            c.setHair(h);
            c.setHeadBand(headBand);
        }
    } else {
        for (size_t i = 0; i < 11 && result; ++i) {
            result &= s & Duel6::Color(c.get(static_cast<Duel6::PlayerSkinColors::BodyPart>(i)));
        }
        result &= s & c.getHair() &&
                  s & c.hasHeadBand();

    }

    return result;

}

#endif

