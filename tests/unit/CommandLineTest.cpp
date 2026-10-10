#include <gtest/gtest.h>
#include "CommandLine.h"

using namespace Duel6;

namespace {
    // CommandLine keeps pointers into argv, like main()'s arguments
    struct Arguments {
        std::vector<std::string> values;
        std::vector<char *> pointers;

        explicit Arguments(std::initializer_list<std::string> args) : values(args) {
            for (auto &value : values) {
                pointers.push_back(&value[0]);
            }
        }

        CommandLine commandLine() {
            return CommandLine(Int32(pointers.size()), pointers.data());
        }
    };
}

TEST(CommandLine, HasFindsFlags) {
    Arguments args{"duel6r", "-dedicated", "-window", "800x600"};
    CommandLine commandLine = args.commandLine();
    EXPECT_TRUE(commandLine.has("-dedicated"));
    EXPECT_TRUE(commandLine.has("-window"));
    EXPECT_FALSE(commandLine.has("-publish"));
}

TEST(CommandLine, GetReturnsFollowingArgument) {
    Arguments args{"duel6r", "-window", "800x600", "-port", "3000"};
    CommandLine commandLine = args.commandLine();
    EXPECT_STREQ(commandLine.get("-window"), "800x600");
    EXPECT_STREQ(commandLine.get("-port"), "3000");
}

TEST(CommandLine, GetReturnsNullForMissingOptionOrValue) {
    Arguments args{"duel6r", "-window"};
    CommandLine commandLine = args.commandLine();
    EXPECT_EQ(commandLine.get("-port"), nullptr);
    EXPECT_EQ(commandLine.get("-window"), nullptr);
}
