#include "grid/GridResource.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace blitzar_grid {

bool GridResourceConfig::IsValid() const noexcept
{
    return dimensions.IsValid() && max_particles > 0 && std::isfinite(padding) && padding > 0.0;
}

bool GridView::IsValid() const noexcept
{
    const std::size_t cells = layout_.CellCount();

    return owner_.has_value() && generation_ != 0 && layout_.IsValid() && cells > 0 &&
           density_.size() == cells && field_x_.size() == cells && field_y_.size() == cells &&
           field_z_.size() == cells;
}

GridDimensions GridView::Dimensions() const noexcept
{
    return layout_.Dimensions();
}

std::size_t GridView::CellCount() const noexcept
{
    return layout_.CellCount();
}

std::uint64_t GridView::Generation() const noexcept
{
    return generation_;
}

const GridLayout& GridView::Layout() const noexcept
{
    return layout_;
}

std::span<const blitzar_core::Scalar> GridView::Density() const noexcept
{
    return density_;
}

std::span<const blitzar_core::Scalar> GridView::FieldX() const noexcept
{
    return field_x_;
}

std::span<const blitzar_core::Scalar> GridView::FieldY() const noexcept
{
    return field_y_;
}

std::span<const blitzar_core::Scalar> GridView::FieldZ() const noexcept
{
    return field_z_;
}

GridResource::GridResource(GridResourceConfig config)
    : config_(config), layout_{}, generation_{}, density_(config.dimensions.CellCount()),
      field_x_(config.dimensions.CellCount()), field_y_(config.dimensions.CellCount()),
      field_z_(config.dimensions.CellCount())
{
}

blitzar_status GridResource::Prepare(blitzar_core::ParticleStateView sources) noexcept
{
    if (!config_.IsValid() || sources.SourceCount() > config_.max_particles ||
        !IsValidState(sources)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    blitzar_core::Vector3 minimum{};
    blitzar_core::Vector3 maximum{};

    if (!BuildBounds(sources, config_.padding, minimum, maximum)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    const GridLayout candidate(config_.dimensions, minimum, maximum);

    if (!candidate.IsValid()) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    layout_ = candidate;

    std::fill(density_.begin(), density_.end(), 0.0);
    std::fill(field_x_.begin(), field_x_.end(), 0.0);
    std::fill(field_y_.begin(), field_y_.end(), 0.0);
    std::fill(field_z_.begin(), field_z_.end(), 0.0);

    for (std::size_t source = 0; source < sources.SourceCount(); ++source) {
        if (!DepositParticle(
                sources.mass[source], {sources.x[source], sources.y[source], sources.z[source]})) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }
    }

    AdvanceGeneration();

    return BLITZAR_STATUS_OK;
}

blitzar_status GridResource::BuildField(const blitzar_physics::GravityLaw& gravity) noexcept
{
    if (!layout_.IsValid() || !gravity.IsValid() || density_.size() != layout_.CellCount()) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    for (std::size_t target = 0; target < layout_.CellCount(); ++target) {
        const blitzar_core::Vector3 target_position = layout_.CellCenter(target);
        blitzar_core::Vector3 field{};

        for (std::size_t source = 0; source < layout_.CellCount(); ++source) {
            const blitzar_core::Scalar mass = density_[source];

            if (mass == 0.0 || source == target) {
                continue;
            }

            const blitzar_core::Vector3 source_position = layout_.CellCenter(source);
            const blitzar_core::Scalar dx = source_position.x - target_position.x;
            const blitzar_core::Scalar dy = source_position.y - target_position.y;
            const blitzar_core::Scalar dz = source_position.z - target_position.z;
            const blitzar_core::Scalar distance_squared = dx * dx + dy * dy + dz * dz;

            if (gravity.ValidatePair(mass, distance_squared) !=
                blitzar_physics::PairStatus::Valid) {
                return BLITZAR_STATUS_INVALID_ARGUMENT;
            }

            const blitzar_core::Scalar factor = gravity.PairFactor(mass, distance_squared);

            if (!std::isfinite(factor)) {
                return BLITZAR_STATUS_INVALID_ARGUMENT;
            }

            field.x += factor * dx;
            field.y += factor * dy;
            field.z += factor * dz;
        }

        if (!std::isfinite(field.x) || !std::isfinite(field.y) || !std::isfinite(field.z)) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }

        field_x_[target] = field.x;
        field_y_[target] = field.y;
        field_z_[target] = field.z;
    }

    return BLITZAR_STATUS_OK;
}

bool GridResource::Interpolate(
    blitzar_core::Vector3 position, blitzar_core::Vector3& field) const noexcept
{
    if (!layout_.IsValid() || density_.size() != layout_.CellCount()) {
        return false;
    }

    GridInterpolation interpolation{};

    if (!layout_.Locate(position, interpolation)) {
        return false;
    }

    field = {};

    for (std::size_t z = 0; z < 2; ++z) {
        const std::size_t z_index = z == 0 ? interpolation.lower.z : interpolation.upper.z;
        const blitzar_core::Scalar z_weight =
            z == 0 ? 1.0 - interpolation.upper_weight.z : interpolation.upper_weight.z;

        for (std::size_t y = 0; y < 2; ++y) {
            const std::size_t y_index = y == 0 ? interpolation.lower.y : interpolation.upper.y;
            const blitzar_core::Scalar y_weight =
                y == 0 ? 1.0 - interpolation.upper_weight.y : interpolation.upper_weight.y;

            for (std::size_t x = 0; x < 2; ++x) {
                const std::size_t x_index = x == 0 ? interpolation.lower.x : interpolation.upper.x;
                const blitzar_core::Scalar x_weight =
                    x == 0 ? 1.0 - interpolation.upper_weight.x : interpolation.upper_weight.x;

                const std::size_t cell = layout_.FlatIndex({x_index, y_index, z_index});
                const blitzar_core::Scalar weight = x_weight * y_weight * z_weight;

                field.x += weight * field_x_[cell];
                field.y += weight * field_y_[cell];
                field.z += weight * field_z_[cell];
            }
        }
    }

    return std::isfinite(field.x) && std::isfinite(field.y) && std::isfinite(field.z);
}

GridView GridResource::View() const noexcept
{
    GridView view{};

    view.layout_ = layout_;
    view.density_ = density_;
    view.field_x_ = field_x_;
    view.field_y_ = field_y_;
    view.field_z_ = field_z_;
    view.generation_ = generation_;
    view.owner_ = std::cref(*this);

    return view;
}

bool GridResource::IsCurrent(GridView view) const noexcept
{
    return view.IsValid() && view.owner_.has_value() && &view.owner_->get() == this &&
           view.generation_ == generation_ && SameLayout(view) &&
           view.density_.size() == density_.size() && view.field_x_.size() == field_x_.size() &&
           view.field_y_.size() == field_y_.size() && view.field_z_.size() == field_z_.size();
}

std::size_t GridResource::MaxParticles() const noexcept
{
    return config_.max_particles;
}

std::size_t GridResource::CellCount() const noexcept
{
    return layout_.CellCount();
}

std::uint64_t GridResource::Generation() const noexcept
{
    return generation_;
}

bool GridResource::IsValidState(blitzar_core::ParticleStateView particles) noexcept
{
    if (!blitzar_core::IsValid(particles)) {
        return false;
    }

    blitzar_core::Scalar total_mass = 0.0;

    for (std::size_t index = 0; index < particles.SourceCount(); ++index) {
        if (!std::isfinite(particles.x[index]) || !std::isfinite(particles.y[index]) ||
            !std::isfinite(particles.z[index]) || !std::isfinite(particles.velocity_x[index]) ||
            !std::isfinite(particles.velocity_y[index]) ||
            !std::isfinite(particles.velocity_z[index]) || !std::isfinite(particles.mass[index]) ||
            particles.mass[index] < 0.0) {
            return false;
        }

        total_mass += particles.mass[index];

        if (!std::isfinite(total_mass)) {
            return false;
        }
    }

    return true;
}

bool GridResource::BuildBounds(blitzar_core::ParticleStateView particles,
    blitzar_core::Scalar padding, blitzar_core::Vector3& minimum,
    blitzar_core::Vector3& maximum) noexcept
{
    if (!std::isfinite(padding) || padding <= 0.0) {
        return false;
    }

    if (particles.SourceCount() == 0) {
        minimum = {-padding, -padding, -padding};
        maximum = {padding, padding, padding};

        return true;
    }

    minimum = {particles.x[0], particles.y[0], particles.z[0]};
    maximum = minimum;

    for (std::size_t index = 1; index < particles.SourceCount(); ++index) {
        minimum.x = std::min(minimum.x, particles.x[index]);
        minimum.y = std::min(minimum.y, particles.y[index]);
        minimum.z = std::min(minimum.z, particles.z[index]);
        maximum.x = std::max(maximum.x, particles.x[index]);
        maximum.y = std::max(maximum.y, particles.y[index]);
        maximum.z = std::max(maximum.z, particles.z[index]);
    }

    minimum.x -= padding;
    minimum.y -= padding;
    minimum.z -= padding;
    maximum.x += padding;
    maximum.y += padding;
    maximum.z += padding;

    return std::isfinite(minimum.x) && std::isfinite(minimum.y) && std::isfinite(minimum.z) &&
           std::isfinite(maximum.x) && std::isfinite(maximum.y) && std::isfinite(maximum.z) &&
           maximum.x > minimum.x && maximum.y > minimum.y && maximum.z > minimum.z;
}

bool GridResource::DepositParticle(
    blitzar_core::Scalar mass, blitzar_core::Vector3 position) noexcept
{
    GridInterpolation interpolation{};

    if (!layout_.Locate(position, interpolation)) {
        return false;
    }

    for (std::size_t z = 0; z < 2; ++z) {
        const std::size_t z_index = z == 0 ? interpolation.lower.z : interpolation.upper.z;
        const blitzar_core::Scalar z_weight =
            z == 0 ? 1.0 - interpolation.upper_weight.z : interpolation.upper_weight.z;

        for (std::size_t y = 0; y < 2; ++y) {
            const std::size_t y_index = y == 0 ? interpolation.lower.y : interpolation.upper.y;
            const blitzar_core::Scalar y_weight =
                y == 0 ? 1.0 - interpolation.upper_weight.y : interpolation.upper_weight.y;

            for (std::size_t x = 0; x < 2; ++x) {
                const std::size_t x_index = x == 0 ? interpolation.lower.x : interpolation.upper.x;
                const blitzar_core::Scalar x_weight =
                    x == 0 ? 1.0 - interpolation.upper_weight.x : interpolation.upper_weight.x;

                const std::size_t cell = layout_.FlatIndex({x_index, y_index, z_index});

                density_[cell] += mass * x_weight * y_weight * z_weight;

                if (!std::isfinite(density_[cell])) {
                    return false;
                }
            }
        }
    }

    return true;
}

bool GridResource::SameLayout(const GridView& view) const noexcept
{
    const GridLayout& candidate = view.Layout();

    const GridDimensions dimensions = layout_.Dimensions();
    const GridDimensions candidate_dimensions = candidate.Dimensions();
    const blitzar_core::Vector3 minimum = layout_.Minimum();
    const blitzar_core::Vector3 candidate_minimum = candidate.Minimum();
    const blitzar_core::Vector3 maximum = layout_.Maximum();
    const blitzar_core::Vector3 candidate_maximum = candidate.Maximum();

    return dimensions.x == candidate_dimensions.x && dimensions.y == candidate_dimensions.y &&
           dimensions.z == candidate_dimensions.z && minimum.x == candidate_minimum.x &&
           minimum.y == candidate_minimum.y && minimum.z == candidate_minimum.z &&
           maximum.x == candidate_maximum.x && maximum.y == candidate_maximum.y &&
           maximum.z == candidate_maximum.z;
}

void GridResource::AdvanceGeneration() noexcept
{
    if (generation_ == std::numeric_limits<std::uint64_t>::max()) {
        generation_ = 1;
    }
    else {
        ++generation_;
    }
}

} // namespace blitzar_grid
