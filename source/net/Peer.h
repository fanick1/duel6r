/*
 * Peer.h
 *
 *  Created on: Mar 9, 2020
 *      Author: fanick
 */

#ifndef SOURCE_NET_PEER_H_
#define SOURCE_NET_PEER_H_
#include <memory>
#include <enet/enet.h>
#include "../Exception.h"
#include "binarystream/BinaryStream.h"
#include "Object.h"
#include "Event.h"
#include "object/Player.h"
#define CHANNEL 0
#define UNRELIABLE_CHANNEL 1
#define SNAPSHOTS 64
namespace Duel6 {
    namespace net {
        class ClientGameProxy;
        class ServerGameProxy;
        class Peer;
        enum class PeerUpdateState {
            WAITING_FOR_REQUEST,
            REQUESTED_GAMESTATE,
            GAMESTATE_RECEIVED,
            RUNNING
        };
        enum class PeerState {
            CONNECTING,
            CONNECTED,
            DISCONNECTING,
            DISCONNECTED,
            DESTROYED
        };
        struct PeerRef { // gets recorded in ENets' peer->data
            size_t pos;
            Peer *peer;
        };
        class Peer {
        private:

            PeerState state = PeerState::DISCONNECTED;
            ClientGameProxy *gameProxy = nullptr;
            ServerGameProxy *serverGameProxy = nullptr; //parent
            ENetHost *host = nullptr;
            std::unique_ptr<ENetPeer> peer;
            size_t pos = 0;
            peer_id_t incomingPeerID = 0;

            // SomeAppObject callbacks
        public:
            const ENetPeer& getEnetPeer() const {
                return *peer;
            }
            PeerUpdateState peerUpdateState = PeerUpdateState::WAITING_FOR_REQUEST;
            Uint32 getRTT() const;
            size_t choke = 64; // 0..64
            std::array<std::map<Uint32, Player>, SNAPSHOTS> snapshot;
            uint16_t confirmedInputsTick = 0; //TODO private
            uint16_t receivedInputsTick = 0;
            Peer(ClientGameProxy &gameProxy, ServerGameProxy &serverGameProxy, ENetPeer *peer, ENetHost *host, size_t pos);
            Peer(ClientGameProxy &gameProxy, ServerGameProxy &serverGameProxy, ENetPeer *peer, ENetHost *host);
            Peer() = default;
            Peer(const Peer &peer) = delete;
            Peer(Peer &&peer) = delete;
            Peer& operator =(Peer &&peer) = default;
            Peer& operator =(const Peer &peer) = default;

            void reset();

            void disconnect();

            template<typename MessageType, typename Stream>
            void handle(MessageType type, Stream &s) {

            }

            template<typename MessageObject>
            void sendReliable(MessageObject &msg, uint8_t channel = CHANNEL) {
                send(msg, CHANNEL, true);
            }

            template<typename MessageObject>
            void sendUnreliable(MessageObject &msg) {
                send(msg, UNRELIABLE_CHANNEL, false);
                enet_host_flush(host);
            }

            template<typename MessageObject>
            void send(MessageObject &msg, uint8_t channel = CHANNEL, bool reliable = true) {
                binarystream bs;
                msg.send(bs);
                std::string dataStr = bs.str();
                const char *data = dataStr.c_str();
                size_t dataLen = dataStr.length();
                ENetPacket *packet = enet_packet_create(data, dataLen, reliable ? ENET_PACKET_FLAG_RELIABLE : 0);
                if(packet == nullptr){
                    D6_THROW(Exception, "Cannot allocate packet");
                    return;
                }
                enet_peer_send(peer.get(), channel, packet);
            }

            template<typename MessageObject>
            void send(MessageObject &&msg) {
                send(msg);
            }

            bool onConnected(ENetPeer *me);

            bool onDisconnected(ENetPeer *me, enet_uint32 reason);

            virtual ~Peer() {
                destroy();
            }

            PeerState getState() {
                return state;
            }

            void requestGameState();

            peer_id_t getIncomingPeerID();

            peer_id_t getClientID();

        protected:
            void disconnect(bool now);

            void destroy();
        };

        template<>
        void Peer::handle(EventType eventType, binarystream &s);

        template<>
        void Peer::handle(ObjectType objectType, binarystream &s);

    } /* namespace net */
} /* namespace Duel6 */

#endif /* SOURCE_NET_PEER_H_ */
