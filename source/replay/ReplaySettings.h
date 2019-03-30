//
// Created by fanick on 30.3.19.
//

#ifndef DUEL6R_REPLAYSETTINGS_H
#define DUEL6R_REPLAYSETTINGS_H

#include "../Type.h"
#include "../GameSettings.h"

namespace Duel6 {
    class Replay;

    class ReplaySettings {
        friend class Replay;

    private:
        std::pair<Int32, Int32> ammoRange;
        Uint32 maxRounds = 0; // e.g. 30
        bool quickLiquid = false;
        bool globalAssistances = false;
        ShotCollisionSetting shotCollision;
        std::set<Uint8> enabledWeapons;
        Int32 gameModeId;
    public:
        ReplaySettings() = default;

        ReplaySettings(const GameSettings &gameSettings,
                       Int32 gameMode) :
                ammoRange(gameSettings.getAmmoRange()),
                maxRounds(gameSettings.getMaxRounds()),
                quickLiquid(gameSettings.isQuickLiquid()),
                globalAssistances(gameSettings.isGlobalAssistances()),
                shotCollision(gameSettings.getShotCollision()),
                enabledWeapons(gameSettings.getEnabledWeapons()),
                gameModeId(gameMode) {
        }

        void configureGameSettings(GameSettings &s, Int32 &gameMode) {
            s.setAmmoRange(ammoRange);
            s.setMaxRounds(maxRounds);
            s.setQuickLiquid(quickLiquid);
            s.setGlobalAssistances(globalAssistances);
            s.setShotCollision(shotCollision);
            s.setEnabledWeapons(enabledWeapons);
            gameMode = gameModeId;
        }

        template<class Stream>
        bool serialize(Stream &s) {
            return
                    s & ammoRange &&
                    s & maxRounds &&
                    s & quickLiquid &&
                    s & globalAssistances &&
                    s & shotCollision &&
                    s & enabledWeapons &&
                    s & gameModeId;
        }

    };

    template<typename Stream>
    bool serialize(Stream &s, ShotCollisionSetting &c) {
        bool result = true;
        if (s.isDeserializer()) {
            Uint8 v;
            result = s.safe_max(v, (Uint8) 2); // 0,1,2
            if (result) {
                c = static_cast<Duel6::ShotCollisionSetting>(v);
            }
        } else {
            result = s & static_cast<Uint8>(c);
        }

        return result;
    }
}

#endif
