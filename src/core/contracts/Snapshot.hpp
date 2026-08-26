#ifndef BLITZAR_CORE_CONTRACTS_SNAPSHOT_HPP
#define BLITZAR_CORE_CONTRACTS_SNAPSHOT_HPP

#include "core/contracts/Types.hpp"

#include <cstdint>

namespace blitzar_core {

inline constexpr std::uint32_t SnapshotMagic = 0x425A5253U;

enum class SnapshotVersion : std::uint16_t { V1 = 1 };

struct SnapshotHeader final {
    std::uint32_t magic{SnapshotMagic};
    SnapshotVersion version{SnapshotVersion::V1};
    std::uint16_t scalar_bytes{sizeof(Scalar)};
    std::uint64_t particle_count{};

    [[nodiscard]] bool IsCompatible() const noexcept
    {
        return magic == SnapshotMagic && version == SnapshotVersion::V1 &&
               scalar_bytes == sizeof(Scalar);
    }
};

} // namespace blitzar_core

#endif
