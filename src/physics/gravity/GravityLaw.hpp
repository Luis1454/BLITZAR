#ifndef BLITZAR_PHYSICS_GRAVITY_GRAVITY_LAW_HPP
#define BLITZAR_PHYSICS_GRAVITY_GRAVITY_LAW_HPP

#include "core/CoreTypes.hpp"
#include "core/CoreUnits.hpp"

namespace blitzar_physics {

enum class PairStatus : unsigned char { Valid = 0, Invalid = 1, Singularity = 2 };

struct GravityParameters final {
    blitzar_core::Scalar gravitational_constant{1.0};
    blitzar_core::Scalar softening{0.0};
    blitzar_core::UnitSystem units{};

    [[nodiscard]] bool IsValid() const noexcept;
    [[nodiscard]] blitzar_core::Scalar EffectiveConstant() const noexcept;
    [[nodiscard]] blitzar_core::Scalar EffectiveSoftening() const noexcept;
};

class GravityLaw final {
public:
    explicit GravityLaw(GravityParameters parameters) noexcept;

    [[nodiscard]] bool IsValid() const noexcept;
    [[nodiscard]] PairStatus ValidatePair(
        blitzar_core::Scalar source_mass, blitzar_core::Scalar squared_distance) const noexcept;
    [[nodiscard]] bool IsValidPair(
        blitzar_core::Scalar source_mass, blitzar_core::Scalar squared_distance) const noexcept;
    [[nodiscard]] blitzar_core::Scalar PairFactor(
        blitzar_core::Scalar source_mass, blitzar_core::Scalar squared_distance) const noexcept;

private:
    GravityParameters parameters_;
    blitzar_core::Scalar effective_constant_;
    blitzar_core::Scalar effective_softening_;
};

} // namespace blitzar_physics

#endif
