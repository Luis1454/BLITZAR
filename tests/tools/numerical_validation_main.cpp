#include "tests/support/numerical_validation_tool.hpp"
#include <iostream>
int main(int argc, char** argv)
{
    grav_test_numerics::NumericalValidationTool tool;
    return tool.run(argc, argv, std::cout, std::cerr);
}
