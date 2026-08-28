#include "io/metadata/MetadataReader.hpp"

#include "io/metadata/MetadataReaderJson.hpp"

#include <cstdint>
#include <fstream>
#include <iterator>
#include <new>
#include <stdexcept>
#include <string>
#include <system_error>

namespace blitzar_io {

namespace {

constexpr std::uintmax_t MaximumManifestBytes = 4U * 1024U * 1024U;

} // namespace

blitzar_status MetadataReader::Read(const std::filesystem::path& path, MetadataRunInfo& info,
    std::vector<std::uint64_t>& completed_steps) const noexcept
{
    if (path.empty()) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    std::error_code file_error;
    const std::uintmax_t file_size = std::filesystem::file_size(path, file_error);

    if (file_error) {
        return BLITZAR_STATUS_INTERNAL_ERROR;
    }

    if (file_size > MaximumManifestBytes) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    try {
        std::ifstream input(path, std::ios::binary);

        if (!input.is_open()) {
            return BLITZAR_STATUS_INTERNAL_ERROR;
        }

        std::string source;

        source.reserve(static_cast<std::size_t>(file_size));
        source.assign(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());

        if (input.bad()) {
            return BLITZAR_STATUS_INTERNAL_ERROR;
        }

        return MetadataReaderJson::Parse(source, info, completed_steps);
    }
    catch (const std::length_error&) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }
    catch (const std::bad_alloc&) {
        return BLITZAR_STATUS_ALLOCATION_FAILURE;
    }
}

} // namespace blitzar_io
