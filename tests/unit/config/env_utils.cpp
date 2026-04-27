// File: tests/unit/config/env_utils.cpp
// Purpose: Verification coverage for the BLITZAR quality gate.

#include "config/EnvUtils.hpp"
#include <gtest/gtest.h>
/// Description: Executes the TEST operation.
TEST(EnvUtilsTest, TST_UNT_CONF_040_ParseBoolReturnsTrueForValidTrues)
{
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(grav_env::parseBool("1", false));
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(grav_env::parseBool("true", false));
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(grav_env::parseBool("TrUe", false));
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(grav_env::parseBool("ON", false));
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(grav_env::parseBool("YES", false));
}
/// Description: Executes the TEST operation.
TEST(EnvUtilsTest, TST_UNT_CONF_041_ParseBoolReturnsFalseForValidFalses)
{
    /// Description: Executes the EXPECT_FALSE operation.
    EXPECT_FALSE(grav_env::parseBool("0", true));
    /// Description: Executes the EXPECT_FALSE operation.
    EXPECT_FALSE(grav_env::parseBool("false", true));
    /// Description: Executes the EXPECT_FALSE operation.
    EXPECT_FALSE(grav_env::parseBool("fAlSe", true));
    /// Description: Executes the EXPECT_FALSE operation.
    EXPECT_FALSE(grav_env::parseBool("OFF", true));
    /// Description: Executes the EXPECT_FALSE operation.
    EXPECT_FALSE(grav_env::parseBool("no", true));
}
/// Description: Executes the TEST operation.
TEST(EnvUtilsTest, TST_UNT_CONF_042_ParseBoolReturnsFallbackForInvalid)
{
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(grav_env::parseBool("invalid", true));
    /// Description: Executes the EXPECT_FALSE operation.
    EXPECT_FALSE(grav_env::parseBool("invalid", false));
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(grav_env::parseBool("", true));
    /// Description: Executes the EXPECT_FALSE operation.
    EXPECT_FALSE(grav_env::parseBool("", false));
}
// Cannot easily mock std::getenv cross-platform for testing 'get' but we can verify it doesn't
// crash on non-existent env variables.
TEST(EnvUtilsTest, TST_UNT_CONF_043_GetReturnsEmptyOptionalForMissingVariable)
{
    const auto value = grav_env::get("GRAVITY_DEFINITELY_MISSING_ENV_VAR_12345");
    /// Description: Executes the EXPECT_FALSE operation.
    EXPECT_FALSE(value.has_value());
}
/// Description: Executes the TEST operation.
TEST(EnvUtilsTest, TST_UNT_CONF_044_GetBoolReturnsFallbackForMissingVariable)
{
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(grav_env::getBool("GRAVITY_MISSING_VAR_BOOL_TEST", true));
    /// Description: Executes the EXPECT_FALSE operation.
    EXPECT_FALSE(grav_env::getBool("GRAVITY_MISSING_VAR_BOOL_TEST", false));
}
/// Description: Executes the TEST operation.
TEST(EnvUtilsTest, TST_UNT_CONF_045_GetNumberReturnsFalseForMissingVariable)
{
    int outInt = 42;
    /// Description: Executes the EXPECT_FALSE operation.
    EXPECT_FALSE(grav_env::getNumber<int>("GRAVITY_MISSING_VAR_NUM_TEST", outInt));
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(outInt, 42); // Value shouldn't change
    float outFloat = 3.14f;
    /// Description: Executes the EXPECT_FALSE operation.
    EXPECT_FALSE(grav_env::getNumber<float>("GRAVITY_MISSING_VAR_NUM_TEST", outFloat));
    /// Description: Executes the EXPECT_FLOAT_EQ operation.
    EXPECT_FLOAT_EQ(outFloat, 3.14f); // Value shouldn't change
}
