#ifndef BLITZAR_PHYSICS_GRAVITY_LAW_HPP
#define BLITZAR_PHYSICS_GRAVITY_LAW_HPP

#include "core/Types.hpp"

namespace blitzar_physics {

struct GravityParameters final {
    blitzar_core::Scalar gravitational_constant{1.0};
    blitzar_core::Scalar softening{0.0};

    [[nodiscard]] bool IsValid() const noexcept;
};

class GravityLaw final {
public:
    explicit GravityLaw(GravityParameters parameters) noexcept;

    [[nodiscard]] bool IsValid() const noexcept;
    [[nodiscard]] blitzar_core::Scalar PairFactor(
        blitzar_core::Scalar source_mass,
        blitzar_core::Scalar squared_distance) const noexcept;

private:
    GravityParameters parameters_;
};

}  // namespace blitzar_physics

#endif
