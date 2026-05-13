/*
 * @file tests/tools/numerical_validation_main.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Automated verification assets for BLITZAR quality gates.
 */

#include "tests/support/numerical_validation_tool.hpp"
#include <iostream>

/*
 * @brief Documents the main operation contract.
 * @param argc Input value used by this contract.
 * @param argv Input value used by this contract.
 * @return int value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
int main(int argc, char** argv)
{
    bltzr_test_numerics::NumericalValidationTool tool;
    return tool.run(argc, argv, std::cout, std::cerr);
}
