#include <gtest/gtest.h>
#include "Color.h"

using namespace Duel6;

TEST(Color, ParsesHexString) {
    Color color = Color::fromString("ff8000");
    EXPECT_EQ(color, Color(255, 128, 0));
    EXPECT_EQ(color.getAlpha(), 255);
}

TEST(Color, WithAlphaKeepsChannels) {
    Color color = Color(10, 20, 30).withAlpha(40);
    EXPECT_EQ(color, Color(10, 20, 30, 40));
}

TEST(Color, ScaleDarkens) {
    EXPECT_EQ(Color(200, 100, 50, 77).scale(0.5f), Color(100, 50, 25, 77));
}

TEST(Color, Equality) {
    EXPECT_EQ(Color(1, 2, 3, 4), Color(1, 2, 3, 4));
    EXPECT_NE(Color(1, 2, 3, 4), Color(1, 2, 3, 5));
}
