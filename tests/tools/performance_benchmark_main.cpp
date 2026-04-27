/*
 * @file tests/tools/performance_benchmark_main.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Automated verification assets for BLITZAR quality gates.
 */

#include "tests/support/performance_benchmark_tool.hpp"
#include <iostream>

/*
 * @brief Documents the main operation contract.
 * @param argc Input value used by this contract.
 * @param argv Input value used by this contract.
 * @return int value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
int main(int argc, const char* const* argv)
{
    grav_test_perf::PerformanceBenchmarkTool tool;
    return tool.run(argc, argv, std::cout, std::cerr);
}
