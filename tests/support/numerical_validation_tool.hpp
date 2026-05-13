/*
 * @file tests/support/numerical_validation_tool.hpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Automated verification assets for BLITZAR quality gates.
 */

#ifndef BLITZAR_TESTS_SUPPORT_NUMERICAL_VALIDATION_TOOL_HPP_
#define BLITZAR_TESTS_SUPPORT_NUMERICAL_VALIDATION_TOOL_HPP_
#include <iosfwd>

namespace bltzr_test_numerics {
class NumericalValidationTool {
public:
    int run(int argc, const char* const* argv, std::ostream& out, std::ostream& err) const;
};
} // namespace bltzr_test_numerics
#endif // BLITZAR_TESTS_SUPPORT_NUMERICAL_VALIDATION_TOOL_HPP_
