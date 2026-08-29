#include "physics/reduction/ScalarReduction.hpp"

#include <cmath>

namespace blitzar_physics {

ScalarReduction::ScalarReduction(ReductionKind kind) noexcept : kind_(kind) {}

void ScalarReduction::Add(blitzar_core::Scalar value) noexcept
{
    switch (kind_) {
    case ReductionKind::Plain:

        sum_ += value;

        return;

    case ReductionKind::Kahan: {
        const blitzar_core::Scalar corrected = value - correction_;
        const blitzar_core::Scalar next = sum_ + corrected;
        correction_ = (next - sum_) - corrected;
        sum_ = next;
        return;
    }

    case ReductionKind::Neumaier: {
        const blitzar_core::Scalar next = sum_ + value;
        correction_ +=
            std::abs(sum_) >= std::abs(value) ? (sum_ - next) + value : (value - next) + sum_;
        sum_ = next;
        return;
    }
    }

    sum_ += value;
}

blitzar_core::Scalar ScalarReduction::Value() const noexcept
{
    return kind_ == ReductionKind::Plain ? sum_ : sum_ + correction_;
}

} // namespace blitzar_physics
