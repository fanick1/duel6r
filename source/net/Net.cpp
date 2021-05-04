/*
 * Net.cpp
 *
 *  Created on: Mar 9, 2020
 *      Author: fanick
 */

#include "Net.h"
#include <enet/enet.h>
#include <iostream> //TODO odjebnout

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
            if (enet_initialize() != 0)
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

