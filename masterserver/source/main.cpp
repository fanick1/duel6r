#include <iostream>
#include <string>
#include <cstdint>
#include <chrono>
#include <ctime>
#include <map>
#include <tuple>
#include <list>
#include <cstring>
#include <enet/enet.h>
#include "../../source/net/binarystream/BinaryStream.h"
#include "masterserver.h"

std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
ENetHost *server;

entryMap hostList;

void sendWaitingNATPeersToServer(ENetPeer *server);

void pushNATPeersToServer(server_list_entry &e);

void addNATPeer(ENetPeer *peer, address_t address, port_t port, address_t clientLocalNetworkAddress, port_t clientLocalNetworkPort) {
    if (!hostList.has(address, port)) {
        printf("The NAT punch request refers to unknown server %s !\n", hostToIPaddress(address, port).c_str());
        return;
    }
    server_list_entry &e = hostList.get(address, port);
    if (!e.needsNAT) {
        // should not happen
        printf("server does not support NAT punch\n");
        return;
    }
    e.registerNatClient(peer->address.host, peer->address.port, clientLocalNetworkAddress, clientLocalNetworkPort);
    pushNATPeersToServer(e);
}

void pushNATPeersToServer(server_list_entry &e) {
    printf("Pushing peers back to the server ... connecting\n");
    ENetAddress address;
    address.host = e.address;
    address.port = e.port;
    ENetPeer *peer = enet_host_connect(server, &address, 1, static_cast<enet_uint32>(REQUEST_TYPE::MASTER_PUSH_NAT_PEERS_TO_SERVER));
    enet_peer_timeout(peer, 100, 100, 1000);
    peer_entry *pe = new peer_entry(PEER_MODE::MASTER_TO_SERVER, now + std::chrono::seconds(2));
    peer->data = (void*) pe;
    pe->connectedCallback = [](ENetPeer *peer) {
        printf("Pushing peers back to the server ... connected\n");
        sendWaitingNATPeersToServer(peer);
    };
}

void sendWaitingNATPeersToServer(ENetPeer *server) {
    server_list_entry &e = hostList.get(server->address.host, server->address.port);
    binarystream s;
    packetHeader header;
    header.type = PACKET_TYPE::SERVER_NAT_PEERS;
    s << header;

    packet_nat_peers p;
    p.yourPublicAddress = e.publicIPAddress != 0 ? e.publicIPAddress : e.address;
    p.yourPublicPort = e.publicPort != 0 ? e.publicPort : e.port;

    std::vector<peer_address_t> addresses = e.scrubNATClients();
    for (auto &c : addresses) {
        packet_nat_peers::_peer peer;
        peer.address = std::get<0>(c);
        peer.port = std::get<1>(c);
        peer.localNetworkAddress = std::get<2>(c);
        peer.localNetworkPort = std::get<3>(c);
        p.peers.push_back(peer);
    }
    p.peerCount = p.peers.size();

    s << p;
    ENetPacket *enetPacket = enet_packet_create(s.str().c_str(), s.tellp(), ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send(server, 0, enetPacket);
}

void sendHostsToPeer(ENetPeer *peer) {
    static std::list<server_list_entry*> validHosts;
    validHosts.clear();
    hostList.getValidHosts(validHosts);
    auto serverCount = validHosts.size();
    binarystream s;
    packetHeader header;
    header.type = PACKET_TYPE::SERVER_LIST;
    s << header;
    packet_serverlist p;

    p.serverCount = serverCount;
    p.servers.reserve(serverCount);

    for (server_list_entry *e : validHosts) {
        packet_serverlist::_serverlist_server server;
        server.address = e->address;
        server.port = e->port;
        server.localNetworkAddress = e->localNetworkAddress;
        server.localNetworkPort = e->localNetworkPort;
        server.publicIPAddress = e->publicIPAddress;
        server.publicPort = e->publicPort;
        server.needsNAT = e->needsNAT;
        server.descr = e->descr;
        p.servers.push_back(server);
    }
    s << p;
    long dataLen = s.tellp();
    ENetPacket *enetPacket = enet_packet_create(s.str().c_str(), dataLen, ENET_PACKET_FLAG_RELIABLE);

    enet_peer_send(peer, 0, enetPacket);
}

void onPacketReceived(ENetPeer *peer, ENetPacket *p) {
    binarystream d(p->data, p->dataLength);
    packetHeader header;
    d >> header;

    switch (header.type) {
    case PACKET_TYPE::SERVER_UPDATE: {
        packet_update s;
        d >> s;
        printf("server %s: update `%s` %s/pub:%s\n",
            hostToIPaddress(peer->address.host, peer->address.port).c_str(), s.descr.c_str(),
            hostToIPaddress(s.localNetworkAddress, s.localNetworkPort).c_str(),
            hostToIPaddress(s.publicIPAddress, s.publicPort).c_str());
        if (s.descr.length() > 100) {
            s.descr = "long PP";
        }
        hostList.update(peer->address.host, peer->address.port,
            s.descr,
            s.localNetworkAddress, s.localNetworkPort,
            s.publicIPAddress, s.publicPort,
            s.needsNAT);
        break;
    }
    default:
        break;
    }
}

void onPeerPacketReceived(ENetPeer *peer, ENetPacket *p) {
    binarystream d(p->data, p->dataLength);
    packetHeader header;
    d >> header;

    switch (header.type) {
    case PACKET_TYPE::CLIENT_NAT_PUNCH: {
        printf("It's a NAT punch request!\n");
        packet_nat_punch s;
        d >> s;
        addNATPeer(peer, s.address, s.port, s.clientLocalNetworkAddress, s.clientLocalNetworkPort);
        enet_peer_disconnect_later(peer, 0);
        break;
    }

    default:
        break;
    }
}
// usage: ./duel6t-masterserver 0.0.0.0 25900   <-- local port (default is 25900)
//                                 ^--------------- local ip address
int main(int argc, char *argv[]) {
    ENetAddress address;
    address.host = ENET_HOST_ANY;
    address.port = 25900; // use high value for the port (NAT traversal might not work with lower ports in netbox.cz network)

    if (argc > 1) {
        std::string addressStr = std::string(argv[1]);
        enet_address_set_host(&address, addressStr.c_str());
    }

    if (argc > 2) {
        std::string portStr = std::string(argv[2]);
        address.port = std::stoi(portStr);
    }

    server = enet_host_create(&address /* the address to bind the server host to */,
        32 /* allow up to 32 clients and/or outgoing connections */,
        1 /* allow up to 1 channels to be used, 0 */,
        0 /* assume any amount of incoming bandwidth */,
        0 /* assume any amount of outgoing bandwidth */);
    if (server == NULL)
    {
        std::cerr << "An error occurred while trying to create an ENet server host.\n";
        exit(EXIT_FAILURE);
    }

    std::cout << "Master local address: " << hostToIPaddress(server->address.host, server->address.port) << "\n";
    ENetEvent event;

    for (;;) {
        now = std::chrono::steady_clock::now();
        hostList.purgeOld();
        for (size_t i = 0; i < server->peerCount; i++) {
            ENetPeer *p = &server->peers[i];
            peer_entry *pe = (peer_entry*) p->data;
            if (p->state == ENetPeerState::ENET_PEER_STATE_CONNECTED && pe->validUntil < now) {
                enet_peer_disconnect_later(p, 0);
            }
        }

        while (enet_host_service(server, &event, 100) > 0) {
            switch (event.type) {
            case ENET_EVENT_TYPE_NONE:
                break;
            case ENET_EVENT_TYPE_CONNECT: {
                REQUEST_TYPE rt = REQUEST_TYPE::NONE;
                if (event.data < static_cast<int>(REQUEST_TYPE::COUNT)) {
                    rt = static_cast<REQUEST_TYPE>(event.data);
                } else {
                    enet_peer_disconnect(event.peer, 255);
                    continue;
                }
                switch (rt) {
                case REQUEST_TYPE::SERVER_REGISTER: {
                    printf("server %s connected\n", hostToIPaddress(event.peer->address.host, event.peer->address.port).c_str());
                    event.peer->data = (void*) new peer_entry(PEER_MODE::SERVER, now + std::chrono::seconds(5));
                    hostList.refresh(event.peer->address.host, event.peer->address.port);
                    enet_peer_disconnect(event.peer, 0);
                    break;
                }
                case REQUEST_TYPE::SERVER_UPDATE: {
                    printf("server %s connected [update]\n", hostToIPaddress(event.peer->address.host, event.peer->address.port).c_str());
                    event.peer->data = (void*) new peer_entry(PEER_MODE::SERVER, now + std::chrono::seconds(5));
                    hostList.refresh(event.peer->address.host, event.peer->address.port);
                    break;
                }
                case REQUEST_TYPE::CLIENT_REQUEST_SERVERLIST: {
                    printf("peer %s requesting server list \n", hostToIPaddress(event.peer->address.host, event.peer->address.port).c_str());
                    event.peer->data = (void*) new peer_entry(PEER_MODE::CLIENT, now + std::chrono::seconds(1));
                    sendHostsToPeer(event.peer);
                    enet_peer_disconnect_later(event.peer, 0);
                    break;
                }
                case REQUEST_TYPE::SERVER_NAT_GET_PEERS: {
                    printf("server %s requesting peers for NAT punch through \n",
                        hostToIPaddress(event.peer->address.host, event.peer->address.port).c_str());
                    event.peer->data = (void*) new peer_entry(PEER_MODE::SERVER, now + std::chrono::seconds(5));
                    hostList.refresh(event.peer->address.host, event.peer->address.port, true);
                    sendWaitingNATPeersToServer(event.peer);
                    enet_peer_disconnect_later(event.peer, 0);
                    break;
                }
                case REQUEST_TYPE::CLIENT_NAT_CONNECT_TO_SERVER: {
                    printf("peer %s requesting NAT punch\n", hostToIPaddress(event.peer->address.host, event.peer->address.port).c_str());
                    event.peer->data = (void*) new peer_entry(PEER_MODE::CLIENT, now + std::chrono::seconds(5));
                    break;
                }

                case REQUEST_TYPE::NONE:
                // possibly outgoing request
                if (event.peer->data != nullptr) {
                    peer_entry *pe = (peer_entry*) event.peer->data;
                    if (pe->mode == PEER_MODE::MASTER_TO_SERVER) {
                        printf("server %s picked up connection to push peers waiting for NAT punch\n",
                            hostToIPaddress(event.peer->address.host, event.peer->address.port).c_str());
                        pe->onConnected(event.peer);
                    }
                }
                //fall through
                case REQUEST_TYPE::COUNT:

                default:
                enet_peer_disconnect_later(event.peer, 0);
                    break;
                }

                break;
            }
            case ENET_EVENT_TYPE_DISCONNECT:
            delete ((peer_entry*) event.peer->data);
            event.peer->data = NULL;
                break;
            case ENET_EVENT_TYPE_RECEIVE:
            peer_entry *pe = (peer_entry*) event.peer->data;
            switch (pe->mode) {
            case PEER_MODE::NONE: {
                break;
            }
            case PEER_MODE::SERVER: {
                onPacketReceived(event.peer, event.packet);
                break;
            }
            case PEER_MODE::CLIENT: {
                onPeerPacketReceived(event.peer, event.packet);
                break;
            }
            case PEER_MODE::MASTER_TO_SERVER: {
                break;
            }
            }
            /* Clean up the packet now that we're done using it. */
            enet_packet_destroy(event.packet);
                break;
            }
        }
    }

    return 0;
}

