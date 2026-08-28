#ifndef BLITZAR_IO_METADATA_METADATA_READER_HPP
#define BLITZAR_IO_METADATA_METADATA_READER_HPP

#include "io/metadata/MetadataManifest.hpp"

#include <cstdint>
#include <filesystem>
#include <vector>

namespace blitzar_io {

class MetadataReader final {
public:
    [[nodiscard]] blitzar_status Read(const std::filesystem::path& path, MetadataRunInfo& info,
        std::vector<std::uint64_t>& completed_steps) const noexcept;
};

} // namespace blitzar_io

#endif
