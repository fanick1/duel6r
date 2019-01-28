#ifndef DUEL6_DEMO_RECORDER_H
#define DUEL6_DEMO_RECORDER_H

#include "Demo.h"

namespace Duel6 {
    class DemoRecorder {
    public:
        const Demo & demo;
        DemoRecorder(const Demo & demo)
        : demo(demo) {

        }

    };
}
#endif
