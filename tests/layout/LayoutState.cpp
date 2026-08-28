#include "layout/LayoutState.hpp"

#include <bit>
#include <cstdint>

namespace blitzar_layout {

namespace {

constexpr std::uint64_t Mix(std::uint64_t value) noexcept
{
    value += 0x9e3779b97f4a7c15ULL;
    value = (value ^ (value >> 30U)) * 0xbf58476d1ce4e5b9ULL;
    value = (value ^ (value >> 27U)) * 0x94d049bb133111ebULL;

    return value ^ (value >> 31U);
}

double Unit(std::uint64_t value) noexcept
{
    return static_cast<double>(value >> 11U) / 9007199254740992.0;
}

} // namespace

LayoutState::LayoutState(std::size_t count, std::uint64_t seed)
    : count_(count), x_(count), y_(count), z_(count), velocity_x_(count), velocity_y_(count),
      velocity_z_(count), mass_(count)
{
    for (std::size_t index = 0; index < count_; ++index) {
        const std::uint64_t base = seed + static_cast<std::uint64_t>(index) * 7U;

        x_[index] = -1.0 + 2.0 * Unit(Mix(base + 0x123456789abcdef0ULL));
        y_[index] = -1.0 + 2.0 * Unit(Mix(base + 0x0fedcba987654321ULL));
        z_[index] = -1.0 + 2.0 * Unit(Mix(base + 0x3141592653589793ULL));
        velocity_x_[index] = 0.01 * (Unit(Mix(base + 1U)) - 0.5);
        velocity_y_[index] = 0.01 * (Unit(Mix(base + 2U)) - 0.5);
        velocity_z_[index] = 0.01 * (Unit(Mix(base + 3U)) - 0.5);
        mass_[index] = 1.0 + 0.25 * static_cast<double>(index % 5U);
    }
}

std::size_t LayoutState::Count() const noexcept
{
    return count_;
}

blitzar_core::ParticleStateView LayoutState::View() const noexcept
{
    return {count_, std::span<const blitzar_core::Scalar>(x_),
        std::span<const blitzar_core::Scalar>(y_), std::span<const blitzar_core::Scalar>(z_),
        std::span<const blitzar_core::Scalar>(velocity_x_),
        std::span<const blitzar_core::Scalar>(velocity_y_),
        std::span<const blitzar_core::Scalar>(velocity_z_),
        std::span<const blitzar_core::Scalar>(mass_)};
}

blitzar_core::Vector3 LayoutState::Minimum() const noexcept
{
    return {-1.0, -1.0, -1.0};
}

blitzar_core::Vector3 LayoutState::Maximum() const noexcept
{
    return {1.0, 1.0, 1.0};
}

std::uint64_t LayoutState::Hash() const noexcept
{
    std::uint64_t hash = 1469598103934665603ULL;
    const auto append = [&hash](const blitzar_core::Scalar value) noexcept {
        hash ^= std::bit_cast<std::uint64_t>(value);
        hash *= 1099511628211ULL;
    };

    const auto view = View();

    for (std::size_t index = 0; index < count_; ++index) {
        append(view.x[index]);
        append(view.y[index]);
        append(view.z[index]);
        append(view.velocity_x[index]);
        append(view.velocity_y[index]);
        append(view.velocity_z[index]);
        append(view.mass[index]);
    }

    return hash;
}

} // namespace blitzar_layout
