// File: tests/support/performance_benchmark_tool.hpp
// Purpose: Verification coverage for the BLITZAR quality gate.

#ifndef GRAVITY_TESTS_SUPPORT_PERFORMANCE_BENCHMARK_TOOL_HPP_
#define GRAVITY_TESTS_SUPPORT_PERFORMANCE_BENCHMARK_TOOL_HPP_
#include <cstdint>
#include <iosfwd>
#include <string>
namespace grav_test_perf {
/// Description: Defines the PerformanceBenchmarkTool data or behavior contract.
class PerformanceBenchmarkTool {
public:
    /// Description: Defines the ToolOptions data or behavior contract.
    struct ToolOptions {
        std::string workload;
        std::string solver = "octree_gpu";
        std::string integrator = "rk4";
        float dt = 0.002f;
        std::uint32_t particleCount = 2048u;
        std::uint32_t steps = 120u;
        std::uint32_t seed = 12345u;
    };
    /// Description: Executes the run operation.
    int run(int argc, const char* const* argv, std::ostream& out, std::ostream& err) const;

private:
    /// Description: Executes the parseArgs operation.
    static bool parseArgs(int argc, const char* const* argv, ToolOptions& out, std::string& error);
    /// Description: Executes the parseFloatValue operation.
    static bool parseFloatValue(const std::string& text, float& out);
    /// Description: Executes the parseUintValue operation.
    static bool parseUintValue(const std::string& text, std::uint32_t& out);
};
} // namespace grav_test_perf
#endif // GRAVITY_TESTS_SUPPORT_PERFORMANCE_BENCHMARK_TOOL_HPP_
