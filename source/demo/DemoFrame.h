#ifndef DUEL6_DEMO_FRAME_H
#define DUEL6_DEMO_FRAME_H

#include <vector>
#include "../Type.h"

namespace Duel6 {
    class DemoFrame {
    public:
        std::vector<Uint32> controllerState; // for each player
        Uint32 frameId = 0;
        DemoFrame();
        DemoFrame(Uint32 frameId);
        DemoFrame(DemoFrame&&);
        DemoFrame & operator =(DemoFrame&&);
        DemoFrame(const DemoFrame&);
        DemoFrame & operator =(const DemoFrame&);
    private:


    };
}

#endif

