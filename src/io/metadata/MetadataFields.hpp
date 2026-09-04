#ifndef BLITZAR_IO_METADATA_METADATA_FIELDS_HPP
#define BLITZAR_IO_METADATA_METADATA_FIELDS_HPP

#include "io/metadata/MetadataCursor.hpp"
#include "io/metadata/MetadataManifest.hpp"

#include <cstdint>
#include <vector>

namespace blitzar_io {

[[nodiscard]] bool ReadMetadataConfiguration(
    MetadataCursor& cursor, MetadataRunConfiguration& configuration);
[[nodiscard]] bool ReadMetadataCapabilities(
    MetadataCursor& cursor, MetadataCapabilities& capabilities);
[[nodiscard]] bool ReadMetadataDistribution(MetadataCursor& cursor, MetadataRunInfo& info);
[[nodiscard]] bool ReadMetadataOutputs(MetadataCursor& cursor, std::uint64_t expected_count,
    MetadataOutputFormat format, std::vector<std::uint64_t>& completed_steps);
[[nodiscard]] bool HasValidMetadataSteps(
    const std::vector<std::uint64_t>& completed_steps, std::uint64_t requested_steps) noexcept;

} // namespace blitzar_io

#endif
