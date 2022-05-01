/*
 * Net.cpp
 *
 *  Created on: Mar 9, 2020
 *      Author: fanick
 */

#include <cstring>
#include "Net.h"
#include <enet/enet.h>
#include <iostream> //TODO odjebnout
#include "../allocator/Malloc.h"

namespace Duel6 {
    namespace net {

        Net::Net() {

        }
        void Net::deinitialize(){
            if(Net::initialized){
                enet_deinitialize();
            }
            Net::initialized = false;
        }
        bool Net::initialize(){
            if(Net::initialized){
                return true;
            }
            ENetCallbacks cb;
            memset(&cb, 0, sizeof(ENetCallbacks));
            cb.free = &Memory::customFree;
            cb.malloc = &Memory::customMalloc;
            if (enet_initialize_with_callbacks(enet_linked_version(), &cb) != 0)
//            if (enet_initialize() != 0)
            {
                std::cerr << "An error occurred while initializing ENet.\n";
                return false;
            }
            Net::initialized = true;
            return true;
        }

        Net::~Net() {
            deinitialize();
        }

        bool Net::initialized = false;

        Net::EnetLibVersion Net::getEnetLibLinkedVersion() const {
            auto version = enet_linked_version();
            return {ENET_VERSION_GET_MAJOR(version), ENET_VERSION_GET_MINOR(version), ENET_VERSION_GET_PATCH(version)};
        }
    } /* namespace net */
} /* namespace Duel6 */

