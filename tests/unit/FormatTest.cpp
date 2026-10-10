#include <gtest/gtest.h>
#include "Format.h"

using namespace Duel6;

TEST(Format, ReplacesPlaceholdersByIndex) {
    std::string text = Format("{1} {0}!") << "world" << "hello";
    EXPECT_EQ(text, "hello world!");
}

TEST(Format, ReplacesEveryOccurrence) {
    std::string text = Format("{0}-{0}") << 7;
    EXPECT_EQ(text, "7-7");
}

TEST(Format, FormatsBasicTypes) {
    std::string text = Format("{0} {1} {2}") << true << 'x' << std::string("s");
    EXPECT_EQ(text, "true x s");
}

TEST(Format, PadsToWidth) {
    EXPECT_EQ(std::string(Format("[{0,5}]") << 42), "[   42]");
    EXPECT_EQ(std::string(Format("[{0,-5}]") << 42), "[42   ]");
    EXPECT_EQ(std::string(Format("[{0,5|0}]") << 42), "[00042]");
}

TEST(Format, DoesNotTruncateLongValues) {
    EXPECT_EQ(std::string(Format("[{0,2}]") << 12345), "[12345]");
}
