#ifndef BLITZAR_IO_SNAPSHOT_SNAPSHOT_CHECKSUM_HPP
#define BLITZAR_IO_SNAPSHOT_SNAPSHOT_CHECKSUM_HPP

#include "core/CoreTypes.hpp"

#include <cstddef>
#include <cstdint>
#include <span>

namespace blitzar_io {

class SnapshotChecksum final {
public:
    SnapshotChecksum() noexcept;

    void Add(std::span<const std::byte> bytes) noexcept;
    void Add(std::uint64_t value) noexcept;
    void Add(blitzar_core::Scalar value) noexcept;

    [[nodiscard]] std::uint64_t Value() const noexcept;

private:
    std::uint64_t value_;
};

} // namespace blitzar_io

#endif
