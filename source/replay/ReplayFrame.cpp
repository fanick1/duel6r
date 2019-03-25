#include <vector>
#include "ReplayFrame.h"

namespace Duel6 {
    ReplayFrame::ReplayFrame(Uint32 frameId) :
            frameId(frameId),
            controllerState() {

    }

    ReplayFrame::ReplayFrame() : ReplayFrame(0) {

    }

    ReplayFrame::ReplayFrame(ReplayFrame &&replayFrame)
            : frameId(replayFrame.frameId),
              controllerState(std::move(replayFrame.controllerState)) {

    }

    ReplayFrame &ReplayFrame::operator=(ReplayFrame &&replayFrame) {
        frameId = replayFrame.frameId;
        controllerState = std::move(replayFrame.controllerState);
        return *this;
    }

    ReplayFrame::ReplayFrame(const ReplayFrame &replayFrame)
            : frameId(replayFrame.frameId),
              controllerState(replayFrame.controllerState) {

    }

    ReplayFrame &ReplayFrame::operator=(const ReplayFrame &replayFrame) {
        frameId = replayFrame.frameId;
        controllerState = replayFrame.controllerState;
        return *this;
    }
}


