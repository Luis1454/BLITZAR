#include "physics/reduction/ScalarReduction.hpp"

#include <cmath>

namespace blitzar_physics {

ScalarReduction::ScalarReduction(ReductionKind kind) noexcept : kind_(kind) {}

ScalarReduction::ScalarReduction(blitzar_core::BackendExecutionPolicy policy) noexcept
    : kind_(policy.reduction == blitzar_core::ReductionPolicy::Compensated ? ReductionKind::Neumaier
                                                                           : ReductionKind::Plain),
      fma_(policy.fma)
{
}

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

void ScalarReduction::AddProduct(blitzar_core::Scalar left, blitzar_core::Scalar right) noexcept
{
    if (kind_ == ReductionKind::Plain && fma_ == blitzar_core::FmaPolicy::Hardware) {
        sum_ = std::fma(left, right, sum_);

        return;
    }

    const blitzar_core::BackendExecutionPolicy product_policy{
        fma_, blitzar_core::ReductionPolicy::Ordered};

    Add(blitzar_core::MultiplyAdd(left, right, 0.0, product_policy));
}

blitzar_core::Scalar ScalarReduction::Value() const noexcept
{
    return kind_ == ReductionKind::Plain ? sum_ : sum_ + correction_;
}

} // namespace blitzar_physics
