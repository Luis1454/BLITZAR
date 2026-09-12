#ifndef BLITZAR_IO_SNAPSHOT_SNAPSHOT_READER_HPP
#define BLITZAR_IO_SNAPSHOT_SNAPSHOT_READER_HPP

#include "core/CoreSnapshot.hpp"

#include <cstddef>
#include <filesystem>
#include <vector>

namespace blitzar_io {

class SnapshotReader final {
public:
    explicit SnapshotReader(std::size_t max_particle_count);

    [[nodiscard]] blitzar_status Read(const std::filesystem::path& path,
        blitzar_core::SnapshotHeader& header, blitzar_core::SnapshotMutablePayloadView payload);

private:
    std::size_t max_particle_count_;
    std::vector<std::byte> buffer_;
};

} // namespace blitzar_io

#endif
