#include "core/CoreSnapshot.hpp"
#include "fixtures/FixtureCheck.hpp"
#include "io/snapshot/SnapshotReader.hpp"
#include "io/snapshot/SnapshotWriter.hpp"

#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <span>
#include <vector>

namespace {

inline constexpr std::size_t SnapshotEndiannessOffset = 40;

struct SnapshotStorage final {
    std::array<std::uint64_t, 2> ids{0, 1};
    std::array<blitzar_core::Scalar, 2> position_x{1.0, 2.0};
    std::array<blitzar_core::Scalar, 2> position_y{3.0, 4.0};
    std::array<blitzar_core::Scalar, 2> position_z{5.0, 6.0};
    std::array<blitzar_core::Scalar, 2> velocity_x{0.1, 0.2};
    std::array<blitzar_core::Scalar, 2> velocity_y{0.3, 0.4};
    std::array<blitzar_core::Scalar, 2> velocity_z{0.5, 0.6};
    std::array<blitzar_core::Scalar, 2> mass{1.0, 2.0};
};

[[nodiscard]] blitzar_core::SnapshotFrameView MakeFrame(const SnapshotStorage& storage) noexcept
{
    blitzar_core::SnapshotHeader header{};

    header.particle_count = storage.ids.size();
    header.step = 11;
    header.time = 2.5;

    const blitzar_core::SnapshotPayloadView payload{std::span<const std::uint64_t>(storage.ids),
        std::span<const blitzar_core::Scalar>(storage.position_x),
        std::span<const blitzar_core::Scalar>(storage.position_y),
        std::span<const blitzar_core::Scalar>(storage.position_z),
        std::span<const blitzar_core::Scalar>(storage.velocity_x),
        std::span<const blitzar_core::Scalar>(storage.velocity_y),
        std::span<const blitzar_core::Scalar>(storage.velocity_z),
        std::span<const blitzar_core::Scalar>(storage.mass)};

    return {header, payload};
}

[[nodiscard]] blitzar_core::SnapshotMutablePayloadView MakeTarget(SnapshotStorage& storage) noexcept
{
    return {std::span<std::uint64_t>(storage.ids),
        std::span<blitzar_core::Scalar>(storage.position_x),
        std::span<blitzar_core::Scalar>(storage.position_y),
        std::span<blitzar_core::Scalar>(storage.position_z),
        std::span<blitzar_core::Scalar>(storage.velocity_x),
        std::span<blitzar_core::Scalar>(storage.velocity_y),
        std::span<blitzar_core::Scalar>(storage.velocity_z),
        std::span<blitzar_core::Scalar>(storage.mass)};
}

[[nodiscard]] std::vector<std::uint8_t> ReadBytes(const std::filesystem::path& path)
{
    std::ifstream input(path, std::ios::binary);

    if (!input.is_open()) {
        return {};
    }

    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

[[nodiscard]] bool WriteBytes(
    const std::filesystem::path& path, std::span<const std::uint8_t> bytes)
{
    std::ofstream output(path, std::ios::binary | std::ios::trunc);

    if (!output.is_open()) {
        return false;
    }

    output.write(
        reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));

    return static_cast<bool>(output);
}

[[nodiscard]] bool SameState(const SnapshotStorage& first, const SnapshotStorage& second) noexcept
{
    return first.ids == second.ids && first.position_x == second.position_x &&
           first.position_y == second.position_y && first.position_z == second.position_z &&
           first.velocity_x == second.velocity_x && first.velocity_y == second.velocity_y &&
           first.velocity_z == second.velocity_z && first.mass == second.mass;
}

int CheckRoundTrip(const SnapshotStorage& input, const std::filesystem::path& first_path,
    const std::filesystem::path& second_path)
{
    const blitzar_core::SnapshotFrameView frame = MakeFrame(input);
    blitzar_io::SnapshotWriter writer(2);

    BLITZAR_CHECK(writer.Write(first_path, frame) == BLITZAR_STATUS_OK);
    BLITZAR_CHECK(writer.Write(second_path, frame) == BLITZAR_STATUS_OK);

    const std::vector<std::uint8_t> first_bytes = ReadBytes(first_path);
    const std::vector<std::uint8_t> second_bytes = ReadBytes(second_path);

    BLITZAR_CHECK(!first_bytes.empty());
    BLITZAR_CHECK(first_bytes == second_bytes);
    BLITZAR_CHECK(first_bytes.size() == blitzar_core::SnapshotWireHeaderBytes +
                                            2U * blitzar_core::SnapshotWireBytesPerParticle +
                                            blitzar_core::SnapshotWireChecksumBytes);

    BLITZAR_CHECK(first_bytes[0] == static_cast<std::uint8_t>('B'));
    BLITZAR_CHECK(first_bytes[1] == static_cast<std::uint8_t>('Z'));
    BLITZAR_CHECK(first_bytes[2] == static_cast<std::uint8_t>('R'));
    BLITZAR_CHECK(first_bytes[3] == static_cast<std::uint8_t>('S'));
    BLITZAR_CHECK(first_bytes[SnapshotEndiannessOffset] == 0U);

    blitzar_io::SnapshotReader reader(2);
    SnapshotStorage output{};
    blitzar_core::SnapshotHeader header{};

    BLITZAR_CHECK(reader.Read(first_path, header, MakeTarget(output)) == BLITZAR_STATUS_OK);
    BLITZAR_CHECK(header.particle_count == frame.header.particle_count);
    BLITZAR_CHECK(header.step == frame.header.step);
    BLITZAR_CHECK(header.time == frame.header.time);
    BLITZAR_CHECK(SameState(input, output));

    return 0;
}

int CheckRejections(
    const std::vector<std::uint8_t>& valid_bytes, const std::filesystem::path& probe_path)
{
    blitzar_io::SnapshotReader reader(2);

    std::vector<std::uint8_t> corrupted = valid_bytes;

    corrupted.back() ^= 1U;

    BLITZAR_CHECK(WriteBytes(probe_path, corrupted));

    SnapshotStorage unchanged{};
    blitzar_core::SnapshotHeader unchanged_header{};

    unchanged_header.step = 99;

    BLITZAR_CHECK(reader.Read(probe_path, unchanged_header, MakeTarget(unchanged)) ==
                  BLITZAR_STATUS_INVALID_ARGUMENT);

    BLITZAR_CHECK(unchanged_header.step == 99);
    BLITZAR_CHECK(unchanged.position_x[0] == 1.0);

    std::vector<std::uint8_t> truncated = valid_bytes;

    truncated.pop_back();

    BLITZAR_CHECK(WriteBytes(probe_path, truncated));
    BLITZAR_CHECK(reader.Read(probe_path, unchanged_header, MakeTarget(unchanged)) ==
                  BLITZAR_STATUS_INVALID_ARGUMENT);

    std::vector<std::uint8_t> incompatible = valid_bytes;

    incompatible[6] = 4;

    BLITZAR_CHECK(WriteBytes(probe_path, incompatible));
    BLITZAR_CHECK(reader.Read(probe_path, unchanged_header, MakeTarget(unchanged)) ==
                  BLITZAR_STATUS_UNSUPPORTED);

    std::vector<std::uint8_t> incompatible_endianness = valid_bytes;

    incompatible_endianness[SnapshotEndiannessOffset] = 1U;

    BLITZAR_CHECK(WriteBytes(probe_path, incompatible_endianness));
    BLITZAR_CHECK(reader.Read(probe_path, unchanged_header, MakeTarget(unchanged)) ==
                  BLITZAR_STATUS_UNSUPPORTED);

    std::vector<std::uint8_t> oversized = valid_bytes;

    oversized.push_back(0);

    BLITZAR_CHECK(WriteBytes(probe_path, oversized));
    BLITZAR_CHECK(reader.Read(probe_path, unchanged_header, MakeTarget(unchanged)) ==
                  BLITZAR_STATUS_INVALID_ARGUMENT);

    std::array<std::uint64_t, 1> small_ids{};

    std::array<blitzar_core::Scalar, 1> small_values{};
    const blitzar_core::SnapshotMutablePayloadView undersized_target{
        std::span<std::uint64_t>(small_ids), std::span<blitzar_core::Scalar>(small_values),
        std::span<blitzar_core::Scalar>(small_values),
        std::span<blitzar_core::Scalar>(small_values),
        std::span<blitzar_core::Scalar>(small_values),
        std::span<blitzar_core::Scalar>(small_values),
        std::span<blitzar_core::Scalar>(small_values),
        std::span<blitzar_core::Scalar>(small_values)};

    BLITZAR_CHECK(WriteBytes(probe_path, valid_bytes));

    BLITZAR_CHECK(reader.Read(probe_path, unchanged_header, undersized_target) ==
                  BLITZAR_STATUS_INVALID_ARGUMENT);

    return 0;
}

} // namespace

int main()
{
    const std::filesystem::path directory = std::filesystem::temp_directory_path();
    const std::filesystem::path first_path = directory / "blitzar-snapshot-642-a.bin";
    const std::filesystem::path second_path = directory / "blitzar-snapshot-642-b.bin";
    const std::filesystem::path probe_path = directory / "blitzar-snapshot-642-probe.bin";

    std::error_code cleanup_error;

    std::filesystem::remove(first_path, cleanup_error);
    std::filesystem::remove(second_path, cleanup_error);
    std::filesystem::remove(probe_path, cleanup_error);

    SnapshotStorage input{};

    BLITZAR_CHECK(CheckRoundTrip(input, first_path, second_path) == 0);

    const std::vector<std::uint8_t> valid_bytes = ReadBytes(first_path);

    BLITZAR_CHECK(CheckRejections(valid_bytes, probe_path) == 0);

    std::filesystem::remove(first_path, cleanup_error);
    std::filesystem::remove(second_path, cleanup_error);
    std::filesystem::remove(probe_path, cleanup_error);

    return 0;
}
