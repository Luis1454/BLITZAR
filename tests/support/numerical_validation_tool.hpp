#pragma once

#include <iosfwd>

namespace grav_test_numerics {

class NumericalValidationTool {
public:
    int run(int argc, const char *const *argv, std::ostream &out, std::ostream &err) const;
};

} // namespace grav_test_numerics

