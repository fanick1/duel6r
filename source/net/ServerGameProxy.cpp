/*
 * ServerGameProxy.cpp
 *
 *  Created on: Mar 21, 2020
 *      Author: fanick
 */

#include "ServerGameProxy.h"
#include "event/events.h"
#include "../Game.h"
namespace Duel6 {
    namespace net {
        uint8_t xor_128 = 127; // % operator yields also negative results
        auto xor_32768 = 0x7fff;
        auto xor_32 = 31;
        const uint16_t xor_64 = 63;

        void ServerGameProxy::add(Peer *p) {
            peers.push_back(p);
        }
        void ServerGameProxy::remove(Peer *p) {
            peers.remove(p);
        }
        ServerGameProxy::ServerGameProxy()
        {
            // TODO Auto-generated constructor stub

        }

        void ServerGameProxy::startRound(Duel6::Level &level) {
            StartRound sr;
            auto &world = sr.world;
            world.elevators.reserve(level.elevators.size());
            for (auto &e : level.elevators) {
                std::vector<Duel6::net::ControlPoint> controlPoints = container_cast<std::vector<Duel6::net::ControlPoint>>(e.controlPoints);
                Elevator elevator(controlPoints,
                    e.circular,
                    e.section,
                    e.remainingWait,
                    e.forward,
                    e.distance,
                    e.travelled,
                    Vector(e.position.x, e.position.y, e.position.z),
                    Vector(e.velocity.x, e.velocity.y, e.velocity.z),
                    e.started);
                sr.world.elevators.emplace_back(std::move(elevator));
            }
            world.width = level.width;
            world.height = level.height;
            world.background = level.background;
            world.waterBlock = level.waterBlock;
            world.levelData = level.levelData;
            world.waterLevel = level.waterLevel;
            world.raisingWater = level.raisingWater;

            for (auto &peer : peers) {
                peer->send(sr); // @suppress("Ambiguous problem")
            }
        }

        ServerGameProxy::~ServerGameProxy()
        {
            peers.clear();
        }

        void ServerGameProxy::handle(ObjectBase &o) {
            ;
        }
        void ServerGameProxy::handle(EventBase &e) {
            ;
        }
        void ServerGameProxy::nextRound() {
            for (auto &peer : peers) {
                for (auto &s : peer->snapshot) {
                    s.clear();
                }
                peer->send(NextRound());
            }
        }

        void ServerGameProxy::sendInputs(Game &game) {
            for (auto &peer : peers) {
                if (!(peer->peerUpdateState == PeerUpdateState::GAMESTATE_RECEIVED || peer->peerUpdateState == PeerUpdateState::RUNNING)) {
                    continue;
                }
                PlayerInputsUpdate piu;
                auto &players = game.getPlayers();

                piu.confirmInputTick = peer->confirmedInputsTick;
                piu.inputTick = game.tick;

                piu.playersInputs.reserve(players.size());
                for (auto &player : players) {
                    if(player.isDeleted()){
                        peer->snapshot[game.tick & xor_64].erase(player.getId());
                    }
                    if (!player.local || player.isDeleted()) {
                        continue;
                    }
                    PlayerInputs pi;
                    pi.id = player.getId();
                    player.unconfirmedInputs[game.tick & xor_128] = player.getControllerState();
                    for (size_t i = 0; i < 64; i++) {
                        pi.unconfirmedInputs[i] = player.unconfirmedInputs[(game.tick - 63 + i) & xor_128];
                    }
                    piu.playersInputs.push_back(pi);
                }
                peer->sendUnreliable(piu);
            }
        }

        void ServerGameProxy::sendGameStateUpdate(Game &game) {
            if (!game.isServer) {
                sendInputs(game);
                return;
            }
            for (auto &peer : peers) {
                if (!(peer->peerUpdateState == PeerUpdateState::GAMESTATE_RECEIVED || peer->peerUpdateState == PeerUpdateState::RUNNING)) {
                    continue;
                }
                GameStateUpdate gsu;
                gsu.confirmInputTick = peer->receivedInputsTick;
                gsu.snapshotTick = peer->confirmedInputsTick;

                gsu.inputTick = game.tick;
                if (gsu.inputTick - gsu.snapshotTick < 32) {
                    // crazy trick that actually solves problem with snapshot divergence
                    // we assume that game state updates get through the network most of the time.
                    // If the peer->confirmedInputsTick starts to diverge (client didn't find correct snapshot and
                    // waits for us to serve him fresh copy of the state), we use that one as the last known snapshot
                    gsu.snapshotTick = gsu.confirmInputTick;
                }
                gsu.players.reserve(game.getPlayers().size());

                for (auto &player : game.getPlayers()) {
                    if(player.isDeleted()){
                        peer->snapshot[game.tick & xor_64].erase(player.getId());
                    }
                    if (!(player.local || game.isServer) || player.isDeleted()) {
                        continue;
                    }
                    Player p;
                    p.rtt = peer->getRTT();
                    p.id = player.getId();
                    p.clientLocalId = player.getClientLocalId();
                    p.debug = game.tick;

//                    if (player.local) {
                        player.unconfirmedInputs[game.tick & xor_128] = player.getControllerState();
                        for (size_t i = 0; i < 64; i++) {
                            p.unconfirmedInputs[i] = player.unconfirmedInputs[(game.tick - 63 + i) & xor_128];
                        }
//                        p.changed[Player::FIELDS::CONTROLS] = true;
//                        p.changed[Player::FIELDS::UNCONFIRMEDINPUTS] = true;
                        p.controls = player.getControllerState();
//                        p.rtt = 0;
//                    }
                    if (game.isServer) {
                        const auto &collidingEntity = player.getCollider();
                        const auto &position = collidingEntity.position;
                        const auto &externalForces = collidingEntity.externalForces;
                        const auto &externalForcesSpeed = collidingEntity.externalForcesSpeed;
                        const auto &velocity = collidingEntity.velocity;
                        const auto &acceleration = collidingEntity.acceleration;
                        p.position = { position.x, position.y };
                        p.externalForces = { externalForces.x, externalForces.y };
                        p.externalForcesSpeed = { externalForcesSpeed.x, externalForcesSpeed.y };
                        p.velocity = { velocity.x, velocity.y };
                        p.acceleration = { acceleration.x, acceleration.y };
                        p.flags = player.getFlags();
                        p.life = player.getLife();
                        p.air = player.getAir();
                        p.ammo = player.getAmmo();
                        p.weaponId = player.getWeapon().getId();
                        p.orientationLeft = { player.getOrientation() == Orientation::Left };

                        peer->snapshot[game.tick & xor_64][p.id] = p;
                    }
                    player.rtt = p.rtt;
                    if (game.isServer &&
                        (((gsu.inputTick - gsu.snapshotTick) & xor_32768) < 64
                            && peer->snapshot[gsu.snapshotTick & xor_64].count(p.id) > 0
                            && peer->snapshot[gsu.snapshotTick & xor_64][p.id].debug == gsu.snapshotTick)) {
                        Player::diff(p, peer->snapshot[gsu.snapshotTick & xor_64][p.id]);
                    }
                    gsu.players.push_back(p);
                }
                peer->sendUnreliable(gsu);
            }
        }

        void ServerGameProxy::playersJoined(std::vector<PlayerDefinition> &playerDefinitions) {
            PlayersJoined pj;
            pj.playerProfiles.reserve(playerDefinitions.size());
            Int32 clientId = 0;
            for (auto &player : playerDefinitions) {
                PlayerProfile cp;
                PlayerProfile::Skin &skin = cp.skin;
                cp.playerId = player.getPlayerId();
                cp.clientLocalId = player.getClientLocalId();
                cp.name = player.getPerson().getName();
                cp.team = player.getTeam();
                cp.clientId = player.getClientId();
                clientId = cp.clientId;
                const PlayerSkinColors &colors = player.getColors();
                skin.hair = (PlayerProfile::Hair) colors.getHair();
                skin.headBand = colors.hasHeadBand();
                size_t i = 0;
                for (auto &c : skin.colors) {
                    PlayerSkinColors::BodyPart bp = (PlayerSkinColors::BodyPart) i;
                    c.red = colors.get(bp).getRed();
                    c.green = colors.get(bp).getGreen();
                    c.blue = colors.get(bp).getBlue();
                    c.alpha = colors.get(bp).getAlpha();

                    i++;
                }
                pj.playerProfiles.push_back(cp);
            }

            for (auto &peer : peers) {
                if(peer->getClientID() != clientId){
                    peer->sendReliable(pj);
                }
            }
        }

        void ServerGameProxy::playersDisconnected(const std::vector<Int32> &ids) {
            PlayersDisconnected pd;
            pd.disconnectedId = ids;
            for (auto &peer : peers) {
                peer->sendReliable(pd);
            }
        }

    } /* namespace net */
} /* namespace Duel6 */
