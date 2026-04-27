// File: tests/unit/config/env_utils.cpp
// Purpose: Verification coverage for the BLITZAR quality gate.

#include "config/EnvUtils.hpp"
#include <gtest/gtest.h>

/// Description: Executes the TEST operation.
TEST(EnvUtilsTest, TST_UNT_CONF_040_ParseBoolReturnsTrueForValidTrues)
{
    EXPECT_TRUE(grav_env::parseBool("1", false));
    EXPECT_TRUE(grav_env::parseBool("true", false));
    EXPECT_TRUE(grav_env::parseBool("TrUe", false));
    EXPECT_TRUE(grav_env::parseBool("ON", false));
    EXPECT_TRUE(grav_env::parseBool("YES", false));
}

/// Description: Executes the TEST operation.
TEST(EnvUtilsTest, TST_UNT_CONF_041_ParseBoolReturnsFalseForValidFalses)
{
    EXPECT_FALSE(grav_env::parseBool("0", true));
    EXPECT_FALSE(grav_env::parseBool("false", true));
    EXPECT_FALSE(grav_env::parseBool("fAlSe", true));
    EXPECT_FALSE(grav_env::parseBool("OFF", true));
    EXPECT_FALSE(grav_env::parseBool("no", true));
}

/// Description: Executes the TEST operation.
TEST(EnvUtilsTest, TST_UNT_CONF_042_ParseBoolReturnsFallbackForInvalid)
{
    EXPECT_TRUE(grav_env::parseBool("invalid", true));
    EXPECT_FALSE(grav_env::parseBool("invalid", false));
    EXPECT_TRUE(grav_env::parseBool("", true));
    EXPECT_FALSE(grav_env::parseBool("", false));
}

// Cannot easily mock std::getenv cross-platform for testing 'get' but we can verify it doesn't
// crash on non-existent env variables.
/// Description: Verifies the TEST behavior.
TEST(EnvUtilsTest, TST_UNT_CONF_043_GetReturnsEmptyOptionalForMissingVariable)
{
    const auto value = grav_env::get("GRAVITY_DEFINITELY_MISSING_ENV_VAR_12345");
    EXPECT_FALSE(value.has_value());
}

/// Description: Executes the TEST operation.
TEST(EnvUtilsTest, TST_UNT_CONF_044_GetBoolReturnsFallbackForMissingVariable)
{
    EXPECT_TRUE(grav_env::getBool("GRAVITY_MISSING_VAR_BOOL_TEST", true));
    EXPECT_FALSE(grav_env::getBool("GRAVITY_MISSING_VAR_BOOL_TEST", false));
}

/// Description: Executes the TEST operation.
TEST(EnvUtilsTest, TST_UNT_CONF_045_GetNumberReturnsFalseForMissingVariable)
{
    int outInt = 42;
    EXPECT_FALSE(grav_env::getNumber<int>("GRAVITY_MISSING_VAR_NUM_TEST", outInt));
    EXPECT_EQ(outInt, 42); // Value shouldn't change
    float outFloat = 3.14f;
    EXPECT_FALSE(grav_env::getNumber<float>("GRAVITY_MISSING_VAR_NUM_TEST", outFloat));
    EXPECT_FLOAT_EQ(outFloat, 3.14f); // Value shouldn't change
}
