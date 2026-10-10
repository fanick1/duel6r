#include <gtest/gtest.h>
#include <fstream>
#include "json/JsonParser.h"
#include "json/JsonWriter.h"

using namespace Duel6;

namespace {
    Json::Value parseText(const std::string &text) {
        std::string path = ::testing::TempDir() + "duel6r_json_test.json";
        std::ofstream(path) << text;
        Json::Parser parser;
        return parser.parse(path);
    }
}

TEST(Json, ParsesAllValueTypes) {
    Json::Value root = parseText(R"({"name": "duel_01", "size": 25, "ratio": 0.5,
                                     "water": true, "missing": null, "items": [1, 2, 3]})");
    ASSERT_EQ(root.getType(), Json::Value::Type::Object);
    EXPECT_EQ(root.get("name").asString(), "duel_01");
    EXPECT_EQ(root.get("size").asInt(), 25);
    EXPECT_DOUBLE_EQ(root.get("ratio").asDouble(), 0.5);
    EXPECT_TRUE(root.get("water").asBoolean());
    EXPECT_EQ(root.get("missing").getType(), Json::Value::Type::Null);
    Json::Value items = root.get("items");
    ASSERT_EQ(items.getLength(), 3u);
    EXPECT_EQ(items.get(Size(2)).asInt(), 3);
}

TEST(Json, GetOrDefaultFallsBack) {
    Json::Value root = parseText(R"({"a": 1})");
    EXPECT_EQ(root.getOrDefault("b", Json::Value::makeNumber(7)).asInt(), 7);
    EXPECT_EQ(root.getOrDefault("a", Json::Value::makeNumber(7)).asInt(), 1);
}

TEST(Json, WrittenTextParsesBack) {
    Json::Value root = Json::Value::makeObject();
    root.set("text", Json::Value::makeString("Player 1: x"));
    root.set("number", Json::Value::makeNumber(-12));
    Json::Value list = Json::Value::makeArray();
    list.add(Json::Value::makeBoolean(false));
    list.add(Json::Value::makeNumber(2.25));
    root.set("list", list);

    for (bool pretty : {false, true}) {
        Json::Writer writer(pretty);
        Json::Value parsed = parseText(writer.writeToString(root));
        EXPECT_EQ(parsed.get("text").asString(), "Player 1: x");
        EXPECT_EQ(parsed.get("number").asInt(), -12);
        EXPECT_FALSE(parsed.get("list").get(Size(0)).asBoolean());
        EXPECT_DOUBLE_EQ(parsed.get("list").get(Size(1)).asDouble(), 2.25);
    }
}

// Known bug: the writer does not escape quotes and backslashes and the parser does not read escapes,
// so e.g. a profile name containing a quote produces an unreadable file. Enable once fixed.
TEST(Json, DISABLED_StringsWithQuotesAndBackslashesRoundTrip) {
    Json::Value root = Json::Value::makeObject();
    root.set("text", Json::Value::makeString("quote \" and backslash \\"));
    Json::Value parsed = parseText(Json::Writer(false).writeToString(root));
    EXPECT_EQ(parsed.get("text").asString(), "quote \" and backslash \\");
}

TEST(Json, MalformedInputThrows) {
    EXPECT_THROW(parseText(R"({"a": )"), Exception);
}
