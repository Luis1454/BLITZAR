#include "grid/GridLayout.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace blitzar_grid {

bool GridDimensions::IsValid() const noexcept
{
    return x > 0 && y > 0 && z > 0 && CellCount() > 0;
}

std::size_t GridDimensions::CellCount() const noexcept
{
    const std::size_t maximum = std::numeric_limits<std::size_t>::max();

    if (x == 0 || y == 0 || z == 0 || x > maximum / y) {
        return 0;
    }

    const std::size_t plane = x * y;

    return plane > maximum / z ? 0 : plane * z;
}

GridLayout::GridLayout(GridDimensions dimensions, blitzar_core::Vector3 minimum,
    blitzar_core::Vector3 maximum) noexcept
    : dimensions_(dimensions), minimum_(minimum), maximum_(maximum), cell_size_{}
{
    const bool finite_bounds = std::isfinite(minimum.x) && std::isfinite(minimum.y) &&
                               std::isfinite(minimum.z) && std::isfinite(maximum.x) &&
                               std::isfinite(maximum.y) && std::isfinite(maximum.z);

    if (dimensions_.IsValid() && finite_bounds && maximum.x > minimum.x && maximum.y > minimum.y &&
        maximum.z > minimum.z) {
        cell_size_ = {(maximum.x - minimum.x) / static_cast<blitzar_core::Scalar>(dimensions.x),
            (maximum.y - minimum.y) / static_cast<blitzar_core::Scalar>(dimensions.y),
            (maximum.z - minimum.z) / static_cast<blitzar_core::Scalar>(dimensions.z)};
    }
}

bool GridLayout::IsValid() const noexcept
{
    return dimensions_.IsValid() && std::isfinite(minimum_.x) && std::isfinite(minimum_.y) &&
           std::isfinite(minimum_.z) && std::isfinite(maximum_.x) && std::isfinite(maximum_.y) &&
           std::isfinite(maximum_.z) && maximum_.x > minimum_.x && maximum_.y > minimum_.y &&
           maximum_.z > minimum_.z && std::isfinite(cell_size_.x) && std::isfinite(cell_size_.y) &&
           std::isfinite(cell_size_.z) && cell_size_.x > 0.0 && cell_size_.y > 0.0 &&
           cell_size_.z > 0.0;
}

GridDimensions GridLayout::Dimensions() const noexcept
{
    return dimensions_;
}

std::size_t GridLayout::CellCount() const noexcept
{
    return dimensions_.CellCount();
}

blitzar_core::Vector3 GridLayout::Minimum() const noexcept
{
    return minimum_;
}

blitzar_core::Vector3 GridLayout::Maximum() const noexcept
{
    return maximum_;
}

blitzar_core::Vector3 GridLayout::CellSize() const noexcept
{
    return cell_size_;
}

std::size_t GridLayout::FlatIndex(GridCoordinate coordinate) const noexcept
{
    if (!IsValid() || coordinate.x >= dimensions_.x || coordinate.y >= dimensions_.y ||
        coordinate.z >= dimensions_.z) {
        return std::numeric_limits<std::size_t>::max();
    }

    return coordinate.x + dimensions_.x * (coordinate.y + dimensions_.y * coordinate.z);
}

blitzar_core::Vector3 GridLayout::CellCenter(std::size_t index) const noexcept
{
    if (!IsValid() || index >= CellCount()) {
        return {};
    }

    const std::size_t plane = dimensions_.x * dimensions_.y;
    const std::size_t z = index / plane;
    const std::size_t remainder = index % plane;
    const std::size_t y = remainder / dimensions_.x;
    const std::size_t x = remainder % dimensions_.x;

    return {minimum_.x + (static_cast<blitzar_core::Scalar>(x) + 0.5) * cell_size_.x,
        minimum_.y + (static_cast<blitzar_core::Scalar>(y) + 0.5) * cell_size_.y,
        minimum_.z + (static_cast<blitzar_core::Scalar>(z) + 0.5) * cell_size_.z};
}

bool GridLayout::Locate(
    blitzar_core::Vector3 position, GridInterpolation& interpolation) const noexcept
{
    if (!IsValid() || !std::isfinite(position.x) || !std::isfinite(position.y) ||
        !std::isfinite(position.z)) {
        return false;
    }

    const blitzar_core::Vector3 bounded{Clamp(position.x, minimum_.x, maximum_.x),
        Clamp(position.y, minimum_.y, maximum_.y), Clamp(position.z, minimum_.z, maximum_.z)};

    const AxisInterpolation x = LocateAxis(bounded.x, minimum_.x, cell_size_.x, dimensions_.x);
    const AxisInterpolation y = LocateAxis(bounded.y, minimum_.y, cell_size_.y, dimensions_.y);
    const AxisInterpolation z = LocateAxis(bounded.z, minimum_.z, cell_size_.z, dimensions_.z);

    interpolation = {{x.lower, y.lower, z.lower}, {x.upper, y.upper, z.upper},
        {x.upper_weight, y.upper_weight, z.upper_weight}};

    return true;
}

blitzar_core::Scalar GridLayout::Clamp(
    blitzar_core::Scalar value, blitzar_core::Scalar minimum, blitzar_core::Scalar maximum) noexcept
{
    return std::min(std::max(value, minimum), maximum);
}

GridLayout::AxisInterpolation GridLayout::LocateAxis(blitzar_core::Scalar value,
    blitzar_core::Scalar minimum, blitzar_core::Scalar cell_size, std::size_t dimension) noexcept
{
    if (dimension <= 1) {
        return {};
    }

    const blitzar_core::Scalar coordinate = (value - minimum) / cell_size - 0.5;
    const std::size_t last = dimension - 1;

    if (coordinate <= 0.0) {
        return {};
    }
    if (coordinate >= static_cast<blitzar_core::Scalar>(last)) {
        return {last, last, 0.0};
    }

    const std::size_t lower = static_cast<std::size_t>(std::floor(coordinate));

    return {lower, lower + 1, coordinate - static_cast<blitzar_core::Scalar>(lower)};
}

} // namespace blitzar_grid
