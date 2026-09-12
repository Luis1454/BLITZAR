#ifndef BLITZAR_IO_SNAPSHOT_SNAPSHOT_DELTA_PAYLOAD_HPP
#define BLITZAR_IO_SNAPSHOT_SNAPSHOT_DELTA_PAYLOAD_HPP

#include "core/CoreSnapshot.hpp"

#include <cstdint>
#include <span>

namespace blitzar_io {

class SnapshotDeltaPayload final {
public:
    [[nodiscard]] static bool Pack(
        blitzar_core::SnapshotPayloadView payload, std::span<std::byte> destination) noexcept;

    [[nodiscard]] static bool Unpack(std::span<const std::byte> source,
        blitzar_core::SnapshotMutablePayloadView destination) noexcept;

    [[nodiscard]] static std::uint64_t Checksum(std::span<const std::byte> bytes) noexcept;
};

} // namespace blitzar_io

#endif
