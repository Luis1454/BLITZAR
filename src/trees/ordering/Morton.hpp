#ifndef BLITZAR_TREES_ORDERING_MORTON_HPP
#define BLITZAR_TREES_ORDERING_MORTON_HPP

#include "core/contracts/Types.hpp"

#include <algorithm>
#include <cstdint>

namespace blitzar_tree_ordering {

inline constexpr int MortonBits = 21;
inline constexpr std::uint32_t MortonMaximum = (std::uint32_t{1} << MortonBits) - std::uint32_t{1};

[[nodiscard]] inline std::uint64_t MortonKey(blitzar_core::Vector3 position,
    blitzar_core::Vector3 minimum, blitzar_core::Vector3 maximum) noexcept
{
    const auto Quantize = [](const blitzar_core::Scalar value, const blitzar_core::Scalar lower,
                              const blitzar_core::Scalar upper) noexcept {
        if (upper <= lower) {
            return std::uint32_t{0};
        }

        const blitzar_core::Scalar normalized = std::clamp((value - lower) / (upper - lower),
            blitzar_core::Scalar{0.0}, blitzar_core::Scalar{1.0});

        return static_cast<std::uint32_t>(normalized * MortonMaximum);
    };

    const std::uint32_t x = Quantize(position.x, minimum.x, maximum.x);
    const std::uint32_t y = Quantize(position.y, minimum.y, maximum.y);
    const std::uint32_t z = Quantize(position.z, minimum.z, maximum.z);
    std::uint64_t key = 0;

    for (int bit = 0; bit < MortonBits; ++bit) {
        key |= static_cast<std::uint64_t>((x >> bit) & 1U) << (3 * bit);
        key |= static_cast<std::uint64_t>((y >> bit) & 1U) << (3 * bit + 1);
        key |= static_cast<std::uint64_t>((z >> bit) & 1U) << (3 * bit + 2);
    }

    return key;
}

} // namespace blitzar_tree_ordering

#endif
