#ifndef BLITZAR_IO_SNAPSHOT_SNAPSHOT_V2_WRITER_HPP
#define BLITZAR_IO_SNAPSHOT_SNAPSHOT_V2_WRITER_HPP

#include "io/snap/codec/SnapshotV2State.hpp"

#include <cstddef>
#include <filesystem>

namespace blitzar_io {

class SnapshotV2Writer final {
public:
    explicit SnapshotV2Writer(std::size_t max_particle_count) noexcept;

    [[nodiscard]] blitzar_status Write(
        const std::filesystem::path& path, const SnapshotV2State& state) const;
    [[nodiscard]] blitzar_status WriteAtomic(
        const std::filesystem::path& path, const SnapshotV2State& state) const;

private:
    std::size_t max_particle_count_;
};

} // namespace blitzar_io

#endif
