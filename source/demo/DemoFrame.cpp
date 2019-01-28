#include <vector>
#include "DemoFrame.h"

namespace Duel6 {
    DemoFrame::DemoFrame(Uint32 frameId) :  controllerState(), frameId(frameId){

    }
    DemoFrame::DemoFrame():DemoFrame(0){

    }
    DemoFrame::DemoFrame(DemoFrame&& demoFrame)
    : frameId(demoFrame.frameId),
      controllerState(std::move(demoFrame.controllerState))
    {

    }
    DemoFrame & DemoFrame::operator =(DemoFrame && demoFrame) {
        frameId = demoFrame.frameId;
        controllerState = std::move(demoFrame.controllerState);
        return *this;
    }
    DemoFrame::DemoFrame(const DemoFrame& demoFrame)
    : frameId(demoFrame.frameId),
      controllerState(demoFrame.controllerState)
    {

    }
    DemoFrame & DemoFrame::operator =(const DemoFrame & demoFrame) {
        frameId = demoFrame.frameId;
        controllerState = demoFrame.controllerState;
        return *this;
    }
}


