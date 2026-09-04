#ifndef BLITZAR_GRID_GRID_RESOURCE_HPP
#define BLITZAR_GRID_GRID_RESOURCE_HPP

#include "core/CoreTypes.hpp"
#include "grid/GridLayout.hpp"
#include "physics/gravity/GravityLaw.hpp"

#include <blitzar/blitzar.h>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include <vector>

namespace blitzar_grid {

struct GridResourceConfig final {
    GridDimensions dimensions{8, 8, 8};
    std::size_t max_particles{};
    blitzar_core::Scalar padding{1.0};

    [[nodiscard]] bool IsValid() const noexcept;
};

struct GridView final {
    [[nodiscard]] bool IsValid() const noexcept;
    [[nodiscard]] GridDimensions Dimensions() const noexcept;
    [[nodiscard]] std::size_t CellCount() const noexcept;
    [[nodiscard]] std::uint64_t Generation() const noexcept;
    [[nodiscard]] const GridLayout& Layout() const noexcept;
    [[nodiscard]] std::span<const blitzar_core::Scalar> Density() const noexcept;
    [[nodiscard]] std::span<const blitzar_core::Scalar> FieldX() const noexcept;
    [[nodiscard]] std::span<const blitzar_core::Scalar> FieldY() const noexcept;
    [[nodiscard]] std::span<const blitzar_core::Scalar> FieldZ() const noexcept;

private:
    GridLayout layout_{};
    std::span<const blitzar_core::Scalar> density_{};
    std::span<const blitzar_core::Scalar> field_x_{};
    std::span<const blitzar_core::Scalar> field_y_{};
    std::span<const blitzar_core::Scalar> field_z_{};
    std::uint64_t generation_{};
    std::optional<std::reference_wrapper<const class GridResource>> owner_{};

    friend class GridResource;
};

class GridResource final {
public:
    explicit GridResource(GridResourceConfig config);

    GridResource(const GridResource&) = delete;
    GridResource& operator=(const GridResource&) = delete;
    GridResource(GridResource&&) noexcept = default;
    GridResource& operator=(GridResource&&) noexcept = default;

    [[nodiscard]] blitzar_status Prepare(blitzar_core::ParticleStateView sources) noexcept;
    [[nodiscard]] blitzar_status BuildField(const blitzar_physics::GravityLaw& gravity) noexcept;
    [[nodiscard]] bool Interpolate(
        blitzar_core::Vector3 position, blitzar_core::Vector3& field) const noexcept;
    [[nodiscard]] GridView View() const noexcept;
    [[nodiscard]] bool IsCurrent(GridView view) const noexcept;
    [[nodiscard]] std::size_t MaxParticles() const noexcept;
    [[nodiscard]] std::size_t CellCount() const noexcept;
    [[nodiscard]] std::uint64_t Generation() const noexcept;

private:
    [[nodiscard]] static bool IsValidState(blitzar_core::ParticleStateView particles) noexcept;
    [[nodiscard]] static bool BuildBounds(blitzar_core::ParticleStateView particles,
        blitzar_core::Scalar padding, blitzar_core::Vector3& minimum,
        blitzar_core::Vector3& maximum) noexcept;
    [[nodiscard]] bool DepositParticle(
        blitzar_core::Scalar mass, blitzar_core::Vector3 position) noexcept;
    [[nodiscard]] bool SameLayout(const GridView& view) const noexcept;
    void AdvanceGeneration() noexcept;

    GridResourceConfig config_{};
    GridLayout layout_{};
    std::uint64_t generation_{};
    std::vector<blitzar_core::Scalar> density_{};
    std::vector<blitzar_core::Scalar> field_x_{};
    std::vector<blitzar_core::Scalar> field_y_{};
    std::vector<blitzar_core::Scalar> field_z_{};
};

} // namespace blitzar_grid

#endif
