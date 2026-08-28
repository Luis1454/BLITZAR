#include "io/metadata/MetadataReaderJson.hpp"

#include "io/metadata/MetadataCursor.hpp"
#include "io/metadata/MetadataFields.hpp"

#include <new>
#include <utility>

namespace blitzar_io {

namespace {

[[nodiscard]] bool ReadDocumentHeader(MetadataCursor& cursor, MetadataRunInfo& info)
{
    std::uint64_t schema_version{};

    if (!cursor.Expect("{") ||
        !cursor.ReadUnsigned("  \"schema_version\": ", ",", schema_version) ||
        schema_version != 1U ||
        !cursor.ReadString("  \"product_version\": ", ",", info.product_version) ||
        !cursor.ReadString("  \"plan_version\": ", ",", info.plan_version)) {
        return false;
    }

    return true;
}

[[nodiscard]] bool ReadDocumentBody(
    MetadataCursor& cursor, MetadataRunInfo& info, std::vector<std::uint64_t>& completed_steps)
{
    std::uint64_t completed_count{};

    if (!ReadMetadataConfiguration(cursor, info.configuration) ||
        !ReadMetadataCapabilities(cursor, info.capabilities) ||
        !ReadMetadataDistribution(cursor, info) ||
        !cursor.ReadUnsigned("  \"completed_output_count\": ", ",", completed_count) ||
        !ReadMetadataOutputs(cursor, completed_count, completed_steps)) {
        return false;
    }

    return completed_steps.size() == completed_count;
}

[[nodiscard]] bool ReadDocumentEnd(MetadataCursor& cursor, const MetadataRunInfo& info,
    const std::vector<std::uint64_t>& completed_steps)
{
    return cursor.Expect("}") && cursor.AtEnd() &&
           HasValidMetadataSteps(completed_steps, info.configuration.simulation.requested_steps);
}

} // namespace

blitzar_status MetadataReaderJson::Parse(std::string_view source, MetadataRunInfo& info,
    std::vector<std::uint64_t>& completed_steps) noexcept
{
    try {
        MetadataCursor cursor(source);
        MetadataRunInfo candidate;
        std::vector<std::uint64_t> candidate_steps;

        if (!ReadDocumentHeader(cursor, candidate) ||
            !ReadDocumentBody(cursor, candidate, candidate_steps) ||
            !ReadDocumentEnd(cursor, candidate, candidate_steps)) {
            return BLITZAR_STATUS_INVALID_ARGUMENT;
        }

        const blitzar_status validation_status = candidate.Validate();

        if (validation_status != BLITZAR_STATUS_OK) {
            return validation_status;
        }

        info = std::move(candidate);
        completed_steps = std::move(candidate_steps);

        return BLITZAR_STATUS_OK;
    }
    catch (const std::length_error&) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    catch (const std::bad_alloc&) {
        return BLITZAR_STATUS_ALLOCATION_FAILURE;
    }
}

} // namespace blitzar_io
