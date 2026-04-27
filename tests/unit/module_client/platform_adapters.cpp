// File: tests/unit/module_client/platform_adapters.cpp
// Purpose: Verification coverage for the BLITZAR quality gate.

#include "platform/DynamicLibrary.hpp"
#include "platform/PlatformProcess.hpp"
#include "platform/SocketPlatform.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <gtest/gtest.h>
#include <string>
#include <vector>
namespace grav_test_module_client_platform_adapters {
TEST(PlatformAdaptersTest, TST_UNT_MODHOST_021_QuoteProcessArgHandlesSpacesAndQuotes)
{
    EXPECT_EQ(grav_platform::quoteProcessArg("simple"), "simple");
    EXPECT_EQ(grav_platform::quoteProcessArg(""), "\"\"");
    EXPECT_EQ(grav_platform::quoteProcessArg("two words"), "\"two words\"");
    EXPECT_EQ(grav_platform::quoteProcessArg("a\"b"), "\"a\\\"b\"");
}
TEST(PlatformAdaptersTest,
     TST_UNT_MODHOST_022_BuildProcessCommandLinePreservesArgumentOrderAndEscaping)
{
    const std::string command = grav_platform::buildProcessCommandLine(
        "C:/Program Files/Blitzar/server.exe",
        {"--mode", "headless", "--name=alpha beta", "--tag=\"x\""});
    EXPECT_NE(command.find("\"C:/Program Files/Blitzar/server.exe\""), std::string::npos);
    EXPECT_NE(command.find(" --mode headless "), std::string::npos);
    EXPECT_NE(command.find("\"--name=alpha beta\""), std::string::npos);
    EXPECT_NE(command.find("\"--tag=\\\"x\\\"\""), std::string::npos);
}
TEST(PlatformAdaptersTest, TST_UNT_MODHOST_023_SocketByteViewsAndTimeoutClampHandleBounds)
{
    std::array<std::byte, 4> writable{};
    grav_socket::MutableBytes mutableBytes{writable.data(), writable.size()};
    EXPECT_FALSE(mutableBytes.empty());
    EXPECT_EQ(mutableBytes.subview(0).size, 4u);
    EXPECT_EQ(mutableBytes.subview(2).size, 2u);
    EXPECT_TRUE(mutableBytes.subview(4).empty());
    const std::array<std::byte, 3> readable{};
    grav_socket::ConstBytes constBytes{readable.data(), readable.size()};
    EXPECT_FALSE(constBytes.empty());
    EXPECT_EQ(constBytes.subview(1).size, 2u);
    EXPECT_TRUE(constBytes.subview(3).empty());
    EXPECT_EQ(grav_socket::clampTimeoutMs(-1), 10);
    EXPECT_EQ(grav_socket::clampTimeoutMs(10), 10);
    EXPECT_EQ(grav_socket::clampTimeoutMs(61000), 60000);
}
TEST(PlatformAdaptersTest, TST_UNT_MODHOST_024_DynamicLibraryRejectsSymbolLookupWhenNotOpen)
{
    grav_platform::DynamicLibrary library;
    std::uintptr_t symbol = 123u;
    std::string error;
    EXPECT_FALSE(library.loadSymbolAddress("entry", symbol, error));
    EXPECT_EQ(symbol, 0u);
    EXPECT_FALSE(error.empty());
    symbol = 999u;
    error.clear();
    EXPECT_FALSE(library.loadSymbolAddress("", symbol, error));
    EXPECT_EQ(symbol, 0u);
    EXPECT_FALSE(error.empty());
}
TEST(PlatformAdaptersTest, TST_UNT_MODHOST_025_ProcessHandleNonRunningTerminateAndClearAreSafe)
{
    grav_platform::ProcessHandle handle;
    std::string error;
    EXPECT_FALSE(handle.isRunning());
    EXPECT_TRUE(handle.terminate(1u, error));
    EXPECT_TRUE(error.empty());
    EXPECT_TRUE(handle.commandLine().empty());
    handle.clear();
    EXPECT_EQ(handle.pidString(), "0");
    EXPECT_TRUE(handle.commandLine().empty());
}
TEST(PlatformAdaptersTest, TST_UNT_MODHOST_026_RunProcessBlockingInvalidExecutableReturnsFailure)
{
    std::string error;
    const int exitCode = grav_platform::runProcessBlocking(
        "C:/definitely_missing/blitzar_missing_binary.exe", {}, false, error);
    EXPECT_NE(exitCode, 0);
    EXPECT_FALSE(error.empty());
}
} // namespace grav_test_module_client_platform_adapters
