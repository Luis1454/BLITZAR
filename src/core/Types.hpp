#ifndef BLITZAR_CORE_TYPES_HPP
#define BLITZAR_CORE_TYPES_HPP

#include <cstddef>

namespace blitzar_core {

using Scalar = double;

struct Vector3 final {
    Scalar x{};
    Scalar y{};
    Scalar z{};
};

struct ParticleView final {
    std::size_t count{};
    const Scalar* x{};
    const Scalar* y{};
    const Scalar* z{};
};

struct ParticleStateView final {
    std::size_t count{};
    const Scalar* x{};
    const Scalar* y{};
    const Scalar* z{};
    const Scalar* velocity_x{};
    const Scalar* velocity_y{};
    const Scalar* velocity_z{};
    const Scalar* mass{};
};

struct MutableParticleView final {
    std::size_t count{};
    Scalar* x{};
    Scalar* y{};
    Scalar* z{};
    Scalar* velocity_x{};
    Scalar* velocity_y{};
    Scalar* velocity_z{};
};

struct ForceView final {
    std::size_t count{};
    Scalar* x{};
    Scalar* y{};
    Scalar* z{};
};

[[nodiscard]] inline bool IsValid(ParticleView view) noexcept
{
    return view.count == 0 || (view.x != nullptr && view.y != nullptr &&
                               view.z != nullptr);
}

[[nodiscard]] inline bool IsValid(ForceView view) noexcept
{
    return view.count == 0 || (view.x != nullptr && view.y != nullptr &&
                               view.z != nullptr);
}

[[nodiscard]] inline bool IsValid(ParticleStateView view) noexcept
{
    return view.count == 0 ||
           (view.x != nullptr && view.y != nullptr && view.z != nullptr &&
            view.velocity_x != nullptr && view.velocity_y != nullptr &&
            view.velocity_z != nullptr && view.mass != nullptr);
}

[[nodiscard]] inline bool IsValid(MutableParticleView view) noexcept
{
    return view.count == 0 ||
           (view.x != nullptr && view.y != nullptr && view.z != nullptr &&
            view.velocity_x != nullptr && view.velocity_y != nullptr &&
            view.velocity_z != nullptr);
}

}  // namespace blitzar_core

#endif
