/*
 * Net.h
 *
 *  Created on: Mar 9, 2020
 *      Author: fanick
 */

#ifndef SOURCE_NET_NET_H_
#define SOURCE_NET_NET_H_
#include <string>
#include <enet/enet.h>
#include "Type.h"

namespace Duel6 {
    namespace net {

        class Net {
            struct EnetLibVersion {
                uint32_t major = 0;
                uint32_t minor = 0;
                uint32_t patch = 0;

                bool operator == (const EnetLibVersion &rhs) const {
                    return major == rhs.major &&
                           minor == rhs.minor &&
                           patch == rhs.patch;
                }
            };
        private:
            static bool initialized;
        public:
            Net();
            bool initialize();
            virtual ~Net();
            [[nodiscard]] constexpr const EnetLibVersion getEnetLibCompileTimeVersion() const {
                return {ENET_VERSION_MAJOR, ENET_VERSION_MINOR, ENET_VERSION_PATCH};
            }
            [[nodiscard]] EnetLibVersion getEnetLibLinkedVersion() const;
        private:
            void deinitialize();
        };

    } /* namespace net */
} /* namespace Duel6 */

#endif /* SOURCE_NET_NET_H_ */
