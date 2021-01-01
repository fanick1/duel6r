/*
 * NetHost.cpp
 *
 *  Created on: Mar 9, 2020
 *      Author: fanick
 */

#include <string>
#include "NetHost.h"
#include "ClientGameProxy.h"
#include "../Game.h"
#include "../console/Console.h"
#include "master/MasterServer.h"
#include "master/protocol.h"
namespace Duel6 {
    namespace net {
#define MAX_PEERS 15
#define CHANNELS 254

        NetHost::NetHost(ClientGameProxy &clientGameProxy, ServerGameProxy &serverGameProxy, Duel6::Console &console)
            : Service(clientGameProxy, serverGameProxy), console(console) {
        }

        NetHost::~NetHost() {
            stop();
        }

        void NetHost::listen(Game &game, const std::string &host,
                             const Duel6::net::port_t port) {
            if (state != ServiceState::UNINITIALIZED) {
                D6_THROW(Exception, "starting server that is not uninitialized");
            }
            setGameReference(game);
            ENetAddress address;
            address.host = ENET_HOST_ANY;
            if (host != "any") {
                enet_address_set_host(&address, host.c_str());
            }
            address.port = port;
            ENetHost *nethost = enet_host_create(&address, MAX_PEERS, CHANNELS, 0, 1000000);

            if (nethost == nullptr) {
                D6_THROW(Exception, "cannot start server (port taken?)");
            }

            console.printLine("NetHost::listen start");
            start(nethost);
            console.printLine("NetHost::listen started ");
            started();
        }

        void NetHost::registerOnMasterServer() {

            if(!enableMasterDiscovery){
                console.printLine(std::string("NetHost::registerOnMasterServer [DISABLED]"));
                return;
            }
            masterserver::MasterServer &proxy = masterServerProxy;

            std::string message = "NetHost::registerOnMasterServer Registering on master server ";
            console.printLine(message);
            auto localAddress = serviceHost.get()->address;
            proxy.update(serviceHost.get(), description, localAddress.host, localAddress.port, publicIPAddress, publicPort, enableNAT);
        }

        void NetHost::natPunch(enet_uint32 address, enet_uint16 port){
            console.printLine(Format("NAT punch! {0}") << hostToIPaddress(address, port));

            ENetAddress peerAddress;
            peerAddress.host = address;
            peerAddress.port = port;
            ENetHost * host = serviceHost.get();
            // fingers crossed
            enet_socket_send(host->socket, &peerAddress, nullptr, 0);
            enet_socket_send(host->socket, &peerAddress, nullptr, 0);
            enet_socket_send(host->socket, &peerAddress, nullptr, 0);
            enet_socket_send(host->socket, &peerAddress, nullptr, 0);
            enet_socket_send(host->socket, &peerAddress, nullptr, 0);
            enet_socket_send(host->socket, &peerAddress, nullptr, 0);
        }

        void NetHost::onNATPeersListReceived(masterserver::peerlist_t & peerList, enet_uint32 publicAddress, enet_uint16 publicPort){
            for (const auto &peer : peerList) {
                natPunch(peer.address, peer.port);
                if (peer.localNetworkAddress != 0 && peer.localNetworkPort != 0) {
                    natPunch(peer.localNetworkAddress, peer.localNetworkPort);
                }
            }
        }

        void NetHost::onStarting() {
            console.printLine("NetHost::onStarting ");
            registerOnMasterServer();
            if(enableMasterDiscovery){
                masterServerProxy.onPeerListReceived([this](masterserver::peerlist_t  & list, enet_uint32 publicAddress, enet_uint16 publicPort){
                    this->onNATPeersListReceived(list, publicAddress, publicPort);
                });
            }
        }

        void NetHost::onStopping() {
            console.printLine("NetHost::onStopping ");
            for (auto &peer : peers) {
                peer->disconnect();
                flush(); // send out the disconnects, don't wait for any replies
                peer->reset();
            }
            flush(); // send out the disconnects, don't wait for any replies
            stopped();
        }

        void NetHost::onStopped() {
            state = ServiceState::STOPPED;
            console.printLine("NetHost::onStopped()");
            peers.clear();
        }

        void NetHost::onPeerConnected(ENetPeer *peer) {
            console.printLine(Format("NetHost::onPeerConnected address: {0}") << hostToIPaddress(peer->address.host, peer->address.port));
            if (state != ServiceState::STARTED) {
                enet_peer_disconnect_now(peer, 42);
            }

            size_t i = 0;
            for (std::unique_ptr<Peer> &p : peers) {
                if (p.get() == nullptr) {
                    p = std::make_unique<Peer>(*clientGameProxy, *serverGameProxy, peer, i, serviceHost.get());
                    p->onConnected(peer);
                    return;
                }
                i++;
            }
            auto &p = peers.emplace_back(std::make_unique<Peer>(*clientGameProxy, *serverGameProxy, peer, i, serviceHost.get()));
            p->onConnected(peer);
        }

        void NetHost::onPeerDisconnected(ENetPeer *peer, enet_uint32 reason) {
            console.printLine("NetHost::onPeerDisconnected");
            PeerRef *peerRef = (PeerRef*) (peer->data);

            if (peerRef->pos + 1 > peers.size()) {
                delete peerRef;
                D6_THROW(Exception, "Server Disconnected peer id out of bounds");
            }

            Peer *p = peers[peerRef->pos].get();
            if (p != peerRef->peer) {

            }
            if (!p->onDisconnected(peer, reason)) {
                return;
            }
            if (state == ServiceState::STOPPING) {
                // this server is stopping
                stopped();
            }
        }

        void NetHost::runPeriodicalTasks(Float64 elapsedTime){
            heartbeatCountDown -= elapsedTime;
            if(heartbeatCountDown <= 0){
                heartbeatCountDown = 20;
                if(enableMasterDiscovery){
                     if(enableNAT){
                         masterServerProxy.requestNatPeers();
                     } else {
                         masterServerProxy.sendHeartBeat();
                     }
                 }
            }
        }

        void NetHost::setServerConfig(enet_uint32 publicIPAddress, enet_uint16 publicPort,
        const std::string & serverDescription, bool enableMasterDiscovery, bool enableNAT ) {
            this->publicIPAddress = publicIPAddress;
            this->publicPort = publicPort;
            this->enableMasterDiscovery = enableMasterDiscovery;
            this->enableNAT = enableNAT;
            description = serverDescription;
        }
    } /* namespace net */
} /* namespace Duel6 */
