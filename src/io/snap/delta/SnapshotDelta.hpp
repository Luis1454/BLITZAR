#ifndef BLITZAR_IO_SNAPSHOT_SNAPSHOT_DELTA_HPP
#define BLITZAR_IO_SNAPSHOT_SNAPSHOT_DELTA_HPP

#include "core/CoreSnapshot.hpp"

#include <cstddef>
#include <cstdint>
#include <span>

namespace blitzar_io {

inline constexpr std::uint32_t SnapshotDeltaMagic = 0x31444C42U;
inline constexpr std::uint16_t SnapshotDeltaVersion = 1;
inline constexpr std::size_t SnapshotDeltaHeaderBytes = 24;
inline constexpr std::size_t SnapshotDeltaMaxRunLength = 128;

struct SnapshotDeltaBuffer final {
    std::span<std::byte> base{};
    std::span<std::byte> current{};
    std::span<std::byte> decoded{};
    std::span<std::byte> encoded{};
    std::size_t encoded_size{};
};

class SnapshotDelta final {
public:
    explicit SnapshotDelta(std::size_t max_particle_count) noexcept;

    [[nodiscard]] std::size_t RawPayloadBytes(std::size_t particle_count) const noexcept;
    [[nodiscard]] std::size_t MaxEncodedBytes(std::size_t particle_count) const noexcept;
    [[nodiscard]] std::size_t WorkspaceBytes(std::size_t particle_count) const noexcept;

    [[nodiscard]] blitzar_status Encode(blitzar_core::SnapshotPayloadView base,
        blitzar_core::SnapshotPayloadView current, SnapshotDeltaBuffer& buffers) const noexcept;

    [[nodiscard]] blitzar_status Decode(blitzar_core::SnapshotPayloadView base,
        SnapshotDeltaBuffer& buffers,
        blitzar_core::SnapshotMutablePayloadView destination) const noexcept;

private:
    std::size_t max_particle_count_;
};

} // namespace blitzar_io

#endif
