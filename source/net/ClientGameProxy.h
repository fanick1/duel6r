/*
 * ClientGameProxy.h
 *
 *  Created on: Mar 21, 2020
 *      Author: fanick
 */

#ifndef SOURCE_NET_CLIENTGAMEPROXY_H_
#define SOURCE_NET_CLIENTGAMEPROXY_H_
#include <list>
#include "GameProxy.h"
#include "Peer.h"
#include "../Person.h"
#include "../PlayerSounds.h"
namespace Duel6 {
    class Game;
    class Player;
    namespace net {
        class Peer;
        const uint8_t xor_128 = 127; // % operator yields also negative results
        const uint16_t xor_32768 = 32767;
        const uint16_t xor_32 = 31;
        const uint16_t xor_64 = 63;

        class ClientGameProxy: public GameProxy {
            Game *game;
            std::list<Person> persons;
            std::map<Int32, Int32> idmap; //player mapping
            PlayerSounds defaultSounds;
            Int32 clientId = 0; // will be set upon receiving gamestate from server
        public:
            void peerDisconnected(Peer &peer);
            bool gameIsServer();
            void lateReceive(tick_t lateTick);
            void netStopped();
            tick_t getTick() const;
            ClientGameProxy() = default;
            virtual ~ClientGameProxy() = default;

            ClientGameProxy(const ClientGameProxy &) = delete;
            ClientGameProxy(ClientGameProxy &&) = delete;
            ClientGameProxy & operator = (const ClientGameProxy &) = delete;
            ClientGameProxy & operator = (ClientGameProxy &&) = delete;

            void setGameReference(Game &g);

            void handle(Player &p);
            void handle(Weapon &w);
            void handle(MessageBroadcast &m);

            void handle(Peer &peer, GameState &s);
            void handle(Peer &peer, GameStateUpdate &s);
            void handle(Peer &peer, PlayerInputsUpdate &piu);
            void handle(NextRound &nr);
            void handle(PlayersDisconnected &pd);
            void handle(PlayersJoined &pj);
            void handle(PlaySample &ps);
            void handle(DoubleJumpEffects &dje);
            void handle(RaiseWaterLevel &rwl);
            void handle(StartRound &sr);
            void handle(SpawnBonus &sb);
            void handle(SpawnWeapon &sb);
            void handle(PickBonus &pb);
            void handle(PickWeapon &pw);
            void handle(SpawnShot &ss);
            void handle(SpawnExplosion &ss);
            void handle(EraseShot &es);
            void handle(RequestNextRound &rnr);
            void handle(DestroyBlocks &db);
            void handle(Peer &peer, ChatMessage &cm);
            void handle(Peer &peer, Chat &c);
            void handle(Peer &peer, net::Console &c);
            void handle(Peer &peer, Focus &f);
            void handle(Peer &peer, RequestGameState &r) override;

            RequestGameState getRequestGameState();

        protected:
            void nextRound() override;

        private:
            void joinPlayers(std::vector<PlayerProfile> &playerProfiles);
        };

    } /* namespace net */
} /* namespace Duel6 */

#endif /* SOURCE_NET_CLIENTGAMEPROXY_H_ */
