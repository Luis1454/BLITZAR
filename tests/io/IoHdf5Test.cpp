#include "core/CoreSnapshot.hpp"
#include "fixtures/FixtureCheck.hpp"
#include "fixtures/FixtureRestart.hpp"
#include "io/hdf5/Hdf5Reader.hpp"
#include "io/hdf5/Hdf5Writer.hpp"

#include <blitzar/blitzar.h>
#if defined(BLITZAR_HAS_HDF5)
#include "io/hdf5/Hdf5Schema.hpp"
#endif

#include <array>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <span>
#include <string>
#include <vector>

namespace {

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
    header.step = 17;
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

[[nodiscard]] bool SameState(const SnapshotStorage& first, const SnapshotStorage& second) noexcept
{
    return first.ids == second.ids && first.position_x == second.position_x &&
           first.position_y == second.position_y && first.position_z == second.position_z &&
           first.velocity_x == second.velocity_x && first.velocity_y == second.velocity_y &&
           first.velocity_z == second.velocity_z && first.mass == second.mass;
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

[[nodiscard]] bool CorruptChecksum(const std::filesystem::path& path)
{
#if defined(BLITZAR_HAS_HDF5)
    const std::string file_name = path.string();
    blitzar_io::Hdf5File file(H5Fopen(file_name.c_str(), H5F_ACC_RDWR, H5P_DEFAULT));

    if (!file.IsValid()) {
        return false;
    }

    blitzar_io::Hdf5Attribute attribute(
        H5Aopen(file.Get(), blitzar_io::Hdf5ChecksumAttribute().name.data(), H5P_DEFAULT));

    std::uint64_t checksum{};

    if (!attribute.IsValid() || H5Aread(attribute.Get(), H5T_NATIVE_UINT64, &checksum) < 0) {
        return false;
    }

    checksum ^= 1U;

    return H5Awrite(attribute.Get(), H5T_NATIVE_UINT64, &checksum) >= 0;
#else

    (void)path;

    return false;
#endif
}

int CheckUnavailable(const std::filesystem::path& path, blitzar_core::SnapshotFrameView frame)
{
    blitzar_io::Hdf5Writer writer(2);
    blitzar_io::Hdf5Reader reader(2);
    SnapshotStorage target{};
    blitzar_core::SnapshotHeader header{};

    header.step = 99;

    BLITZAR_CHECK(!blitzar_io::Hdf5Writer::IsAvailable());
    BLITZAR_CHECK(!blitzar_io::Hdf5Reader::IsAvailable());
    BLITZAR_CHECK(writer.Write(path, frame) == BLITZAR_STATUS_UNSUPPORTED);
    BLITZAR_CHECK(writer.WriteAtomic(path, frame) == BLITZAR_STATUS_UNSUPPORTED);
    BLITZAR_CHECK(reader.Read(path, header, MakeTarget(target)) == BLITZAR_STATUS_UNSUPPORTED);
    BLITZAR_CHECK(header.step == 99);
    BLITZAR_CHECK(!std::filesystem::exists(path));

    return 0;
}

int CheckCapabilityBoundary() noexcept
{
    blitzar_capabilities_v2 capabilities{sizeof(capabilities), BLITZAR_ABI_VERSION_V2, 0, 0, 0, 0};

    BLITZAR_CHECK(blitzar_get_capabilities_v2(&capabilities) == BLITZAR_STATUS_OK);

    const bool hdf5_deferred = (capabilities.deferred_feature_mask & BLITZAR_FEATURE_HDF5) != 0;

    BLITZAR_CHECK(hdf5_deferred == !blitzar_io::Hdf5Writer::IsAvailable());

    return 0;
}

int CheckRoundTrip(const std::filesystem::path& directory, const SnapshotStorage& input,
    blitzar_core::SnapshotFrameView frame)
{
    const std::filesystem::path first = directory / "first.h5";
    const std::filesystem::path second = directory / "second.h5";
    const std::filesystem::path atomic = directory / "atomic.h5";
    const std::filesystem::path restart = directory / "restart.h5";
    const std::filesystem::path corrupted = directory / "corrupted.h5";
    const std::filesystem::path truncated = directory / "truncated.h5";

    blitzar_io::Hdf5Writer writer(2);

    BLITZAR_CHECK(blitzar_io::Hdf5Writer::IsAvailable());
    BLITZAR_CHECK(blitzar_io::Hdf5Reader::IsAvailable());
    BLITZAR_CHECK(writer.Write(first, frame) == BLITZAR_STATUS_OK);
    BLITZAR_CHECK(writer.Write(second, frame) == BLITZAR_STATUS_OK);
    BLITZAR_CHECK(writer.WriteAtomic(atomic, frame) == BLITZAR_STATUS_OK);
    BLITZAR_CHECK(!std::filesystem::exists(atomic.string() + ".tmp"));

    const std::vector<std::uint8_t> first_bytes = ReadBytes(first);

    BLITZAR_CHECK(!first_bytes.empty());
    BLITZAR_CHECK(first_bytes == ReadBytes(second));
    BLITZAR_CHECK(first_bytes == ReadBytes(atomic));

    SnapshotStorage output{};
    blitzar_core::SnapshotHeader header{};
    blitzar_io::Hdf5Reader reader(2);

    BLITZAR_CHECK(reader.Read(first, header, MakeTarget(output)) == BLITZAR_STATUS_OK);
    BLITZAR_CHECK(header.step == frame.header.step);
    BLITZAR_CHECK(header.time == frame.header.time);
    BLITZAR_CHECK(SameState(input, output));

    const blitzar_core::SnapshotFrameView restart_frame = MakeFrame(output);

    BLITZAR_CHECK(writer.Write(restart, restart_frame) == BLITZAR_STATUS_OK);
    BLITZAR_CHECK(first_bytes == ReadBytes(restart));

    BLITZAR_CHECK(WriteBytes(corrupted, first_bytes));
    BLITZAR_CHECK(CorruptChecksum(corrupted));

    output.position_x[0] = 99.0;
    header.step = 99;

    BLITZAR_CHECK(reader.Read(corrupted, header, MakeTarget(output)) != BLITZAR_STATUS_OK);
    BLITZAR_CHECK(header.step == 99);
    BLITZAR_CHECK(output.position_x[0] == 99.0);

    std::vector<std::uint8_t> truncated_bytes = ReadBytes(corrupted);

    BLITZAR_CHECK(!truncated_bytes.empty());

    truncated_bytes.pop_back();

    BLITZAR_CHECK(WriteBytes(truncated, truncated_bytes));
    BLITZAR_CHECK(reader.Read(truncated, header, MakeTarget(output)) != BLITZAR_STATUS_OK);

    return 0;
}

int CheckRepeatedWrites(const std::filesystem::path& directory,
    blitzar_core::SnapshotFrameView frame, std::size_t repeat_count)
{
    blitzar_io::Hdf5Writer writer(2);
    const std::filesystem::path baseline = directory / "baseline.h5";

    BLITZAR_CHECK(writer.Write(baseline, frame) == BLITZAR_STATUS_OK);

    const std::vector<std::uint8_t> baseline_bytes = ReadBytes(baseline);

    for (std::size_t index = 0; index < repeat_count; ++index) {
        const std::filesystem::path path = directory / ("repeat-" + std::to_string(index) + ".h5");

        BLITZAR_CHECK(writer.Write(path, frame) == BLITZAR_STATUS_OK);
        BLITZAR_CHECK(ReadBytes(path) == baseline_bytes);
    }

    return 0;
}

} // namespace

int main()
{
    std::filesystem::path directory;

    BLITZAR_CHECK(blitzar_test::AcquireTestDirectory(directory, "blitzar-hdf5-650-"));
    BLITZAR_CHECK(CheckCapabilityBoundary() == 0);

    const SnapshotStorage input{};
    const blitzar_core::SnapshotFrameView frame = MakeFrame(input);
    const auto start = std::chrono::steady_clock::now();
    const bool checks_passed = blitzar_io::Hdf5Writer::IsAvailable()
                                   ? CheckRoundTrip(directory, input, frame) == 0 &&
                                         CheckRepeatedWrites(directory, frame, 32) == 0
                                   : CheckUnavailable(directory / "unavailable.h5", frame) == 0;

    const auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now() - start);

    const std::size_t file_bytes =
        blitzar_io::Hdf5Writer::IsAvailable() ? ReadBytes(directory / "baseline.h5").size() : 0;

    const std::size_t staging_bytes =
        frame.header.particle_count * blitzar_core::SnapshotWireBytesPerParticle;

    std::cout << "hdf5-evidence backend="
              << (blitzar_io::Hdf5Writer::IsAvailable() ? "enabled" : "unavailable")
              << " schema=1 soa_fields=8 repeat_writes=32 transactional_read=1"
              << " file_bytes=" << file_bytes << " staging_bytes=" << staging_bytes
              << " elapsed_us=" << elapsed.count() << "\n";

    BLITZAR_CHECK(checks_passed);
    BLITZAR_CHECK(blitzar_test::RemoveTree(directory));

    return 0;
}
