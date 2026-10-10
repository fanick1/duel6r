#include <gtest/gtest.h>
#include <string>
#include <vector>
#include "automation/Timeline.h"
#include "Defines.h"

using namespace Duel6;

namespace {
    void runTicks(Timeline &timeline, Uint32 count) {
        for (Uint32 i = 0; i < count; i++) {
            timeline.tick();
        }
    }
}

TEST(Timeline, StartActionsRunOnStartOnly) {
    Timeline timeline;
    Int32 runs = 0;
    timeline.atStart([&runs]() { runs++; });
    runTicks(timeline, 10);
    EXPECT_EQ(runs, 0);
    timeline.start();
    EXPECT_EQ(runs, 1);
}

TEST(Timeline, ActionRunsOnceAtItsTick) {
    Timeline timeline;
    std::vector<Uint32> ranAt;
    timeline.at(1.0, [&]() { ranAt.push_back(timeline.getTick()); });
    runTicks(timeline, D6_UPDATE_FREQUENCY - 1);
    EXPECT_TRUE(ranAt.empty());
    runTicks(timeline, 3 * D6_UPDATE_FREQUENCY);
    ASSERT_EQ(ranAt.size(), 1u);
    EXPECT_EQ(ranAt[0], Uint32(D6_UPDATE_FREQUENCY));
}

TEST(Timeline, ActionsDueOnTheSameTickRunInInsertionOrder) {
    Timeline timeline;
    std::string order;
    timeline.at(0.5, [&]() { order += "a"; });
    timeline.at(0.5, [&]() { order += "b"; });
    timeline.at(0.2, [&]() { order += "c"; });
    runTicks(timeline, D6_UPDATE_FREQUENCY);
    EXPECT_EQ(order, "cab");
}

TEST(Timeline, ZeroSecondsRunsOnFirstTick) {
    Timeline timeline;
    bool ran = false;
    timeline.at(0, [&]() { ran = true; });
    timeline.tick();
    EXPECT_TRUE(ran);
    EXPECT_EQ(timeline.getTick(), 1u);
}
