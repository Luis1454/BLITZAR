#ifndef BLITZAR_IO_METADATA_METADATA_READER_JSON_HPP
#define BLITZAR_IO_METADATA_METADATA_READER_JSON_HPP

#include "io/metadata/MetadataManifest.hpp"

#include <cstdint>
#include <string_view>
#include <vector>

namespace blitzar_io {

class MetadataReaderJson final {
public:
    [[nodiscard]] static blitzar_status Parse(std::string_view source, MetadataRunInfo& info,
        std::vector<std::uint64_t>& completed_steps) noexcept;
};

} // namespace blitzar_io

#endif
