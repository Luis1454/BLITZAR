// File: tests/tools/numerical_validation_main.cpp
// Purpose: Verification coverage for the BLITZAR quality gate.

#include "tests/support/numerical_validation_tool.hpp"
#include <iostream>

/// Description: Executes the main operation.
int main(int argc, char** argv)
{
    grav_test_numerics::NumericalValidationTool tool;
    return tool.run(argc, argv, std::cout, std::cerr);
}
