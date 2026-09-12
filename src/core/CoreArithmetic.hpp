#ifndef BLITZAR_CORE_CORE_ARITHMETIC_HPP
#define BLITZAR_CORE_CORE_ARITHMETIC_HPP

#include "core/CoreExecution.hpp"
#include "core/CoreTypes.hpp"

#include <cmath>

namespace blitzar_core {

[[nodiscard]] inline Scalar MultiplyAdd(
    Scalar left, Scalar right, Scalar addend, const BackendExecutionPolicy& policy) noexcept
{
    if (policy.fma == FmaPolicy::Hardware) {
        return std::fma(left, right, addend);
    }

    volatile Scalar product = left * right;

    return addend + product;
}

} // namespace blitzar_core

#endif
