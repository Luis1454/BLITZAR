#ifndef BLITZAR_IO_HDF5_HDF5_READER_HPP
#define BLITZAR_IO_HDF5_HDF5_READER_HPP

#include "core/CoreSnapshot.hpp"

#include <cstddef>
#include <filesystem>

namespace blitzar_io {

class Hdf5Reader final {
public:
    explicit Hdf5Reader(std::size_t max_particle_count) noexcept;

    [[nodiscard]] static bool IsAvailable() noexcept;

    [[nodiscard]] blitzar_status Read(const std::filesystem::path& path,
        blitzar_core::SnapshotHeader& header,
        blitzar_core::SnapshotMutablePayloadView payload) const;

private:
    std::size_t max_particle_count_;
};

} // namespace blitzar_io

#endif
