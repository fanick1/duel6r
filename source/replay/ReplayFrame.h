#ifndef DUEL6_DEMO_FRAME_H
#define DUEL6_DEMO_FRAME_H

#include <vector>
#include "../Type.h"

namespace Duel6 {
    class ReplayFrame {
    public:
        Uint32 frameId = 0;
        std::vector<Uint32> controllerState; // for each player

        ReplayFrame();

        ReplayFrame(Uint32 frameId);

        ReplayFrame(ReplayFrame &&);

        ReplayFrame &operator=(ReplayFrame &&);

        ReplayFrame(const ReplayFrame &);

        ReplayFrame &operator=(const ReplayFrame &);

        template<class Stream>
        bool serialize(Stream &s) {
            return s & frameId &&
                   s & controllerState;
        }

    private:


    };
}

#endif

