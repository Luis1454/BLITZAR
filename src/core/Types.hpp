#ifndef BLITZAR_CORE_TYPES_HPP
#define BLITZAR_CORE_TYPES_HPP

#include <cstddef>
#include <span>

namespace blitzar_core {

using Scalar = double;

struct Vector3 final {
    Scalar x{};
    Scalar y{};
    Scalar z{};
};

struct ParticleView final {
    std::size_t count{};
    std::span<const Scalar> x{};
    std::span<const Scalar> y{};
    std::span<const Scalar> z{};
};

struct ParticleStateView final {
    std::size_t count{};
    std::span<const Scalar> x{};
    std::span<const Scalar> y{};
    std::span<const Scalar> z{};
    std::span<const Scalar> velocity_x{};
    std::span<const Scalar> velocity_y{};
    std::span<const Scalar> velocity_z{};
    std::span<const Scalar> mass{};
    std::size_t source_count{};

    [[nodiscard]] std::size_t SourceCount() const noexcept
    {
        return source_count == 0 ? count : source_count;
    }
};

struct MutableParticleView final {
    std::size_t count{};
    std::span<Scalar> x{};
    std::span<Scalar> y{};
    std::span<Scalar> z{};
    std::span<Scalar> velocity_x{};
    std::span<Scalar> velocity_y{};
    std::span<Scalar> velocity_z{};
};

struct ForceView final {
    std::size_t count{};
    std::span<Scalar> x{};
    std::span<Scalar> y{};
    std::span<Scalar> z{};
};

[[nodiscard]] inline bool IsValid(ParticleView view) noexcept
{
    return view.count == view.x.size() && view.count == view.y.size() &&
           view.count == view.z.size();
}

[[nodiscard]] inline bool IsValid(ForceView view) noexcept
{
    return view.count == view.x.size() && view.count == view.y.size() &&
           view.count == view.z.size();
}

[[nodiscard]] inline bool IsValid(ParticleStateView view) noexcept
{
    const std::size_t source_count = view.SourceCount();
    return view.count <= source_count && source_count == view.x.size() &&
           source_count == view.y.size() && source_count == view.z.size() &&
           source_count == view.velocity_x.size() &&
           source_count == view.velocity_y.size() &&
           source_count == view.velocity_z.size() &&
           source_count == view.mass.size();
}

[[nodiscard]] inline bool IsValid(MutableParticleView view) noexcept
{
    return view.count == view.x.size() && view.count == view.y.size() &&
           view.count == view.z.size() &&
           view.count == view.velocity_x.size() &&
           view.count == view.velocity_y.size() &&
           view.count == view.velocity_z.size();
}

}  // namespace blitzar_core

#endif
