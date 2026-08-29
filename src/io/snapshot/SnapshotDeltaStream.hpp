#ifndef BLITZAR_IO_SNAPSHOT_SNAPSHOT_DELTA_STREAM_HPP
#define BLITZAR_IO_SNAPSHOT_SNAPSHOT_DELTA_STREAM_HPP

#include "io/snapshot/SnapshotDelta.hpp"

#include <cstdint>
#include <span>

namespace blitzar_io {

struct SnapshotDeltaStreamHeader final {
    std::uint64_t raw_bytes{};
    std::uint64_t checksum{};
};

class SnapshotDeltaStream final {
public:
    [[nodiscard]] static bool WriteHeader(std::span<std::byte> destination, std::size_t& position,
        SnapshotDeltaStreamHeader header) noexcept;

    [[nodiscard]] static bool ReadHeader(std::span<const std::byte> source, std::size_t& position,
        SnapshotDeltaStreamHeader& header) noexcept;

    [[nodiscard]] static bool EncodeRuns(std::span<const std::byte> base,
        std::span<const std::byte> current, std::span<std::byte> destination,
        std::size_t& position) noexcept;

    [[nodiscard]] static bool DecodeRuns(std::span<const std::byte> base,
        std::span<const std::byte> source, std::span<std::byte> decoded,
        std::size_t& position) noexcept;
};

} // namespace blitzar_io

#endif
