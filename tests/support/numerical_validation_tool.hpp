// File: tests/support/numerical_validation_tool.hpp
// Purpose: Verification coverage for the BLITZAR quality gate.

#ifndef GRAVITY_TESTS_SUPPORT_NUMERICAL_VALIDATION_TOOL_HPP_
#define GRAVITY_TESTS_SUPPORT_NUMERICAL_VALIDATION_TOOL_HPP_
#include <iosfwd>
namespace grav_test_numerics {
class NumericalValidationTool {
public:
    int run(int argc, const char* const* argv, std::ostream& out, std::ostream& err) const;
};
} // namespace grav_test_numerics
#endif // GRAVITY_TESTS_SUPPORT_NUMERICAL_VALIDATION_TOOL_HPP_
