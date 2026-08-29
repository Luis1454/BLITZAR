#ifndef BLITZAR_PHYSICS_REDUCTION_SCALAR_REDUCTION_HPP
#define BLITZAR_PHYSICS_REDUCTION_SCALAR_REDUCTION_HPP

#include "core/CoreTypes.hpp"

#include <cstdint>

namespace blitzar_physics {

enum class ReductionKind : std::uint8_t { Plain, Kahan, Neumaier };

class ScalarReduction final {
public:
    explicit ScalarReduction(ReductionKind kind) noexcept;

    void Add(blitzar_core::Scalar value) noexcept;
    [[nodiscard]] blitzar_core::Scalar Value() const noexcept;

private:
    ReductionKind kind_;
    blitzar_core::Scalar sum_{};
    blitzar_core::Scalar correction_{};
};

} // namespace blitzar_physics

#endif
