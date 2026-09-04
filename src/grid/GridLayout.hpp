#ifndef BLITZAR_GRID_GRID_LAYOUT_HPP
#define BLITZAR_GRID_GRID_LAYOUT_HPP

#include "core/CoreTypes.hpp"

#include <cstddef>

namespace blitzar_grid {

struct GridDimensions final {
    std::size_t x{};
    std::size_t y{};
    std::size_t z{};

    [[nodiscard]] bool IsValid() const noexcept;
    [[nodiscard]] std::size_t CellCount() const noexcept;
};

struct GridCoordinate final {
    std::size_t x{};
    std::size_t y{};
    std::size_t z{};
};

struct GridInterpolation final {
    GridCoordinate lower{};
    GridCoordinate upper{};
    blitzar_core::Vector3 upper_weight{};
};

class GridLayout final {
public:
    GridLayout() noexcept = default;
    GridLayout(GridDimensions dimensions, blitzar_core::Vector3 minimum,
        blitzar_core::Vector3 maximum) noexcept;

    [[nodiscard]] bool IsValid() const noexcept;
    [[nodiscard]] GridDimensions Dimensions() const noexcept;
    [[nodiscard]] std::size_t CellCount() const noexcept;
    [[nodiscard]] blitzar_core::Vector3 Minimum() const noexcept;
    [[nodiscard]] blitzar_core::Vector3 Maximum() const noexcept;
    [[nodiscard]] blitzar_core::Vector3 CellSize() const noexcept;
    [[nodiscard]] std::size_t FlatIndex(GridCoordinate coordinate) const noexcept;
    [[nodiscard]] blitzar_core::Vector3 CellCenter(std::size_t index) const noexcept;
    [[nodiscard]] bool Locate(
        blitzar_core::Vector3 position, GridInterpolation& interpolation) const noexcept;

private:
    struct AxisInterpolation final {
        std::size_t lower{};
        std::size_t upper{};
        blitzar_core::Scalar upper_weight{};
    };

    [[nodiscard]] static blitzar_core::Scalar Clamp(blitzar_core::Scalar value,
        blitzar_core::Scalar minimum, blitzar_core::Scalar maximum) noexcept;
    [[nodiscard]] static AxisInterpolation LocateAxis(blitzar_core::Scalar value,
        blitzar_core::Scalar minimum, blitzar_core::Scalar cell_size,
        std::size_t dimension) noexcept;

    GridDimensions dimensions_{};
    blitzar_core::Vector3 minimum_{};
    blitzar_core::Vector3 maximum_{};
    blitzar_core::Vector3 cell_size_{};
};

} // namespace blitzar_grid

#endif
