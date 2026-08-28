#include "fixtures/FixtureRestart.hpp"

#include "core/CoreSnapshot.hpp"
#include "io/snapshot/SnapshotWire.hpp"

#include <bit>
#include <new>
#include <stdexcept>
#include <string>

namespace blitzar_test {

namespace {

constexpr std::size_t SnapshotMassFieldIndex = 7U;
constexpr std::size_t TestDirectoryAttempts = 1024U;

bool RefreshChecksum(std::vector<std::uint8_t>& bytes)
{
    if (bytes.size() <
        blitzar_core::SnapshotWireHeaderBytes + blitzar_core::SnapshotWireChecksumBytes) {
        return false;
    }

    const std::size_t checksum_offset = bytes.size() - blitzar_core::SnapshotWireChecksumBytes;
    const std::span<const std::uint8_t> content =
        std::span<const std::uint8_t>(bytes).first(checksum_offset);

    blitzar_io::SnapshotWireReader wire(std::as_bytes(content));

    for (std::size_t index = 0; index < blitzar_core::SnapshotWireHeaderBytes; ++index) {
        std::uint8_t value{};

        if (!wire.ReadUnhashed(value)) {
            return false;
        }
    }

    while (!wire.AtEnd()) {
        std::uint8_t value{};

        if (!wire.Read(value)) {
            return false;
        }
    }

    const auto encoded = blitzar_io::EncodeLittleEndian(wire.Checksum());

    for (std::size_t index = 0; index < encoded.size(); ++index) {
        bytes[checksum_offset + index] = std::to_integer<unsigned char>(encoded[index]);
    }

    return true;
}

} // namespace

bool EnsureDirectory(const std::filesystem::path& path) noexcept
{
    std::error_code error;

    std::filesystem::create_directories(path, error);

    return !error && std::filesystem::is_directory(path, error) && !error;
}

bool AcquireTestDirectory(std::filesystem::path& directory, std::string_view prefix) noexcept
{
    try {
        const std::filesystem::path parent = std::filesystem::temp_directory_path();

        for (std::size_t index = 0; index < TestDirectoryAttempts; ++index) {
            const std::filesystem::path candidate =
                parent / (std::string(prefix) + std::to_string(index));

            std::error_code error;

            if (std::filesystem::create_directory(candidate, error)) {
                directory = candidate;

                return true;
            }

            if (error) {
                return false;
            }
        }
    }
    catch (const std::filesystem::filesystem_error&) {
        return false;
    }
    catch (const std::length_error&) {
        return false;
    }
    catch (const std::bad_alloc&) {
        return false;
    }

    return false;
}

bool RemoveTree(const std::filesystem::path& path) noexcept
{
    std::error_code error;

    std::filesystem::remove_all(path, error);

    return !error;
}

bool SetMassAndRefreshChecksum(
    std::vector<std::uint8_t>& bytes, std::size_t particle_index, double mass)
{
    const std::size_t payload_offset = blitzar_core::SnapshotWireHeaderBytes;
    const std::size_t checksum_bytes = blitzar_core::SnapshotWireChecksumBytes;

    if (bytes.size() < payload_offset + checksum_bytes) {
        return false;
    }

    const std::size_t particle_count = (bytes.size() - payload_offset - checksum_bytes) /
                                       blitzar_core::SnapshotWireBytesPerParticle;

    if (particle_index >= particle_count) {
        return false;
    }

    const std::size_t mass_offset =
        payload_offset + SnapshotMassFieldIndex * particle_count * sizeof(blitzar_core::Scalar) +
        particle_index * sizeof(blitzar_core::Scalar);

    const auto encoded = blitzar_io::EncodeLittleEndian(std::bit_cast<std::uint64_t>(mass));

    for (std::size_t index = 0; index < encoded.size(); ++index) {
        bytes[mass_offset + index] = std::to_integer<unsigned char>(encoded[index]);
    }

    return RefreshChecksum(bytes);
}

} // namespace blitzar_test
