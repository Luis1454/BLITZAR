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
/// Description: Executes the TEST operation.
TEST(PlatformAdaptersTest, TST_UNT_MODHOST_021_QuoteProcessArgHandlesSpacesAndQuotes)
{
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(grav_platform::quoteProcessArg("simple"), "simple");
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(grav_platform::quoteProcessArg(""), "\"\"");
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(grav_platform::quoteProcessArg("two words"), "\"two words\"");
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(grav_platform::quoteProcessArg("a\"b"), "\"a\\\"b\"");
}
TEST(PlatformAdaptersTest,
     TST_UNT_MODHOST_022_BuildProcessCommandLinePreservesArgumentOrderAndEscaping)
{
    const std::string command = grav_platform::buildProcessCommandLine(
        "C:/Program Files/Blitzar/server.exe",
        {"--mode", "headless", "--name=alpha beta", "--tag=\"x\""});
    /// Description: Executes the EXPECT_NE operation.
    EXPECT_NE(command.find("\"C:/Program Files/Blitzar/server.exe\""), std::string::npos);
    /// Description: Executes the EXPECT_NE operation.
    EXPECT_NE(command.find(" --mode headless "), std::string::npos);
    /// Description: Executes the EXPECT_NE operation.
    EXPECT_NE(command.find("\"--name=alpha beta\""), std::string::npos);
    /// Description: Executes the EXPECT_NE operation.
    EXPECT_NE(command.find("\"--tag=\\\"x\\\"\""), std::string::npos);
}
/// Description: Executes the TEST operation.
TEST(PlatformAdaptersTest, TST_UNT_MODHOST_023_SocketByteViewsAndTimeoutClampHandleBounds)
{
    std::array<std::byte, 4> writable{};
    grav_socket::MutableBytes mutableBytes{writable.data(), writable.size()};
    /// Description: Executes the EXPECT_FALSE operation.
    EXPECT_FALSE(mutableBytes.empty());
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(mutableBytes.subview(0).size, 4u);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(mutableBytes.subview(2).size, 2u);
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(mutableBytes.subview(4).empty());
    const std::array<std::byte, 3> readable{};
    grav_socket::ConstBytes constBytes{readable.data(), readable.size()};
    /// Description: Executes the EXPECT_FALSE operation.
    EXPECT_FALSE(constBytes.empty());
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(constBytes.subview(1).size, 2u);
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(constBytes.subview(3).empty());
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(grav_socket::clampTimeoutMs(-1), 10);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(grav_socket::clampTimeoutMs(10), 10);
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(grav_socket::clampTimeoutMs(61000), 60000);
}
/// Description: Executes the TEST operation.
TEST(PlatformAdaptersTest, TST_UNT_MODHOST_024_DynamicLibraryRejectsSymbolLookupWhenNotOpen)
{
    grav_platform::DynamicLibrary library;
    std::uintptr_t symbol = 123u;
    std::string error;
    /// Description: Executes the EXPECT_FALSE operation.
    EXPECT_FALSE(library.loadSymbolAddress("entry", symbol, error));
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(symbol, 0u);
    /// Description: Executes the EXPECT_FALSE operation.
    EXPECT_FALSE(error.empty());
    symbol = 999u;
    error.clear();
    /// Description: Executes the EXPECT_FALSE operation.
    EXPECT_FALSE(library.loadSymbolAddress("", symbol, error));
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(symbol, 0u);
    /// Description: Executes the EXPECT_FALSE operation.
    EXPECT_FALSE(error.empty());
}
/// Description: Executes the TEST operation.
TEST(PlatformAdaptersTest, TST_UNT_MODHOST_025_ProcessHandleNonRunningTerminateAndClearAreSafe)
{
    grav_platform::ProcessHandle handle;
    std::string error;
    /// Description: Executes the EXPECT_FALSE operation.
    EXPECT_FALSE(handle.isRunning());
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(handle.terminate(1u, error));
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(error.empty());
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(handle.commandLine().empty());
    handle.clear();
    /// Description: Executes the EXPECT_EQ operation.
    EXPECT_EQ(handle.pidString(), "0");
    /// Description: Executes the EXPECT_TRUE operation.
    EXPECT_TRUE(handle.commandLine().empty());
}
/// Description: Executes the TEST operation.
TEST(PlatformAdaptersTest, TST_UNT_MODHOST_026_RunProcessBlockingInvalidExecutableReturnsFailure)
{
    std::string error;
    const int exitCode = grav_platform::runProcessBlocking(
        "C:/definitely_missing/blitzar_missing_binary.exe", {}, false, error);
    /// Description: Executes the EXPECT_NE operation.
    EXPECT_NE(exitCode, 0);
    /// Description: Executes the EXPECT_FALSE operation.
    EXPECT_FALSE(error.empty());
}
} // namespace grav_test_module_client_platform_adapters
