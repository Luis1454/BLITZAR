/*
 * @file tests/unit/config/env_utils.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Automated verification assets for BLITZAR quality gates.
 */

#include "config/EnvUtils.hpp"
#include <gtest/gtest.h>

TEST(EnvUtilsTest, TST_UNT_CONF_040_ParseBoolReturnsTrueForValidTrues)
{
    EXPECT_TRUE(bltzr_env::parseBool("1", false));
    EXPECT_TRUE(bltzr_env::parseBool("true", false));
    EXPECT_TRUE(bltzr_env::parseBool("TrUe", false));
    EXPECT_TRUE(bltzr_env::parseBool("ON", false));
    EXPECT_TRUE(bltzr_env::parseBool("YES", false));
}

TEST(EnvUtilsTest, TST_UNT_CONF_041_ParseBoolReturnsFalseForValidFalses)
{
    EXPECT_FALSE(bltzr_env::parseBool("0", true));
    EXPECT_FALSE(bltzr_env::parseBool("false", true));
    EXPECT_FALSE(bltzr_env::parseBool("fAlSe", true));
    EXPECT_FALSE(bltzr_env::parseBool("OFF", true));
    EXPECT_FALSE(bltzr_env::parseBool("no", true));
}

TEST(EnvUtilsTest, TST_UNT_CONF_042_ParseBoolReturnsFallbackForInvalid)
{
    EXPECT_TRUE(bltzr_env::parseBool("invalid", true));
    EXPECT_FALSE(bltzr_env::parseBool("invalid", false));
    EXPECT_TRUE(bltzr_env::parseBool("", true));
    EXPECT_FALSE(bltzr_env::parseBool("", false));
}

// Cannot easily mock std::getenv cross-platform for testing 'get' but we can verify it doesn't
// crash on non-existent env variables.
TEST(EnvUtilsTest, TST_UNT_CONF_043_GetReturnsEmptyOptionalForMissingVariable)
{
    const auto value = bltzr_env::get("BLITZAR_DEFINITELY_MISSING_ENV_VAR_12345");
    EXPECT_FALSE(value.has_value());
}

TEST(EnvUtilsTest, TST_UNT_CONF_044_GetBoolReturnsFallbackForMissingVariable)
{
    EXPECT_TRUE(bltzr_env::getBool("BLITZAR_MISSING_VAR_BOOL_TEST", true));
    EXPECT_FALSE(bltzr_env::getBool("BLITZAR_MISSING_VAR_BOOL_TEST", false));
}

TEST(EnvUtilsTest, TST_UNT_CONF_045_GetNumberReturnsFalseForMissingVariable)
{
    int outInt = 42;
    EXPECT_FALSE(bltzr_env::getNumber<int>("BLITZAR_MISSING_VAR_NUM_TEST", outInt));
    EXPECT_EQ(outInt, 42); // Value shouldn't change
    float outFloat = 3.14f;
    EXPECT_FALSE(bltzr_env::getNumber<float>("BLITZAR_MISSING_VAR_NUM_TEST", outFloat));
    EXPECT_FLOAT_EQ(outFloat, 3.14f); // Value shouldn't change
}
