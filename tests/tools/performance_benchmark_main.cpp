// File: tests/tools/performance_benchmark_main.cpp
// Purpose: Verification coverage for the BLITZAR quality gate.

#include "tests/support/performance_benchmark_tool.hpp"
#include <iostream>
int main(int argc, const char* const* argv)
{
    grav_test_perf::PerformanceBenchmarkTool tool;
    return tool.run(argc, argv, std::cout, std::cerr);
}
