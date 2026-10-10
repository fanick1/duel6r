#include <gtest/gtest.h>
#include "math/Math.h"
#include "math/Vector.h"

using namespace Duel6;

namespace {
    void expectVector(const Vector &actual, Float32 x, Float32 y, Float32 z) {
        EXPECT_FLOAT_EQ(actual.x, x);
        EXPECT_FLOAT_EQ(actual.y, y);
        EXPECT_FLOAT_EQ(actual.z, z);
    }
}

TEST(Math, AngleDiffTakesTheShorterWay) {
    EXPECT_FLOAT_EQ(Math::angleDiff(10, 350), 20);
    EXPECT_FLOAT_EQ(Math::angleDiff(90, 0), 90);
    EXPECT_FLOAT_EQ(Math::angleDiff(0, 180), 180);
}

TEST(Math, DegreeRadianConversion) {
    EXPECT_NEAR(Math::degToRad(180.0f), 3.14159265f, 1e-5f);
    EXPECT_NEAR(Math::radToDeg(Math::degToRad(37.0f)), 37.0f, 1e-4f);
}

TEST(Math, Sign) {
    EXPECT_EQ(Math::sign(-3), -1);
    EXPECT_EQ(Math::sign(5), 1);
}

TEST(Math, SeedMakesRandomReproducible) {
    Math::seed(1234);
    Int32 a = Math::random(1000000);
    Float32 b = Math::random(0.0f, 1.0f);
    Math::seed(1234);
    EXPECT_EQ(Math::random(1000000), a);
    EXPECT_FLOAT_EQ(Math::random(0.0f, 1.0f), b);
}

TEST(Math, RandomStaysInRange) {
    Math::seed(1);
    for (Int32 i = 0; i < 1000; i++) {
        Int32 value = Math::random(3, 7);
        EXPECT_GE(value, 3);
        EXPECT_LE(value, 7);
    }
}

TEST(Vector, Arithmetic) {
    Vector a(1, 2, 3), b(4, 5, 6);
    expectVector(a + b, 5, 7, 9);
    expectVector(b - a, 3, 3, 3);
    expectVector(a * 2.0f, 2, 4, 6);
    EXPECT_FLOAT_EQ(a.dot(b), 32);
    expectVector(Vector(1, 0, 0).cross(Vector(0, 1, 0)), 0, 0, 1);
}

TEST(Vector, LengthAndUnit) {
    Vector v(3, 4, 0);
    EXPECT_FLOAT_EQ(v.length(), 5);
    EXPECT_FLOAT_EQ(v.unit().length(), 1);
}
