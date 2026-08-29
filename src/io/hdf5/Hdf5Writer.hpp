#ifndef BLITZAR_IO_HDF5_HDF5_WRITER_HPP
#define BLITZAR_IO_HDF5_HDF5_WRITER_HPP

#include "core/CoreSnapshot.hpp"

#include <cstddef>
#include <filesystem>

namespace blitzar_io {

class Hdf5Writer final {
public:
    explicit Hdf5Writer(std::size_t max_particle_count) noexcept;

    [[nodiscard]] static bool IsAvailable() noexcept;

    [[nodiscard]] blitzar_status Write(
        const std::filesystem::path& path, blitzar_core::SnapshotFrameView frame) const;

    [[nodiscard]] blitzar_status WriteAtomic(
        const std::filesystem::path& path, blitzar_core::SnapshotFrameView frame) const;

private:
    std::size_t max_particle_count_;
};

} // namespace blitzar_io

#endif
