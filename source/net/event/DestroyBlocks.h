/*
 * EraseShot.h
 *
 *  Created on: May 4, 2021
 *      Author: fanick
 */

#ifndef SOURCE_NET_EVENT_DESTROYBLOCKS_H_
#define SOURCE_NET_EVENT_DESTROYBLOCKS_H_

#include "../Event.h"
#include "../Type.h"
namespace Duel6::net {

    class DestroyBlocks: public Event<DestroyBlocks, EventType::DESTROY_BLOCKS> {
    public:
        std::vector<Vector2D> blocks;

        template<typename Stream>
        bool serialize(Stream &s){
            return s & blocks;
        }
    };
}



#endif /* SOURCE_NET_EVENT_ERASESHOT_H_ */
