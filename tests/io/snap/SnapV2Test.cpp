#include "fixtures/FixtureCheck.hpp"
#include "io/md/manifest/MetadataManifest.hpp"
#include "io/snap/codec/SnapshotRepartition.hpp"
#include "io/snap/codec/SnapshotV2Reader.hpp"
#include "io/snap/codec/SnapshotV2State.hpp"
#include "io/snap/codec/SnapshotV2Writer.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <span>
#include <string_view>
#include <vector>

namespace {

inline constexpr std::size_t ParticleCount = 4;

struct V2Storage final {
    std::array<std::uint64_t, ParticleCount> ids{1, 2, 3, 4};
    std::array<blitzar_core::Scalar, ParticleCount> position_x{1.0, 2.0, 3.0, 4.0};
    std::array<blitzar_core::Scalar, ParticleCount> position_y{5.0, 6.0, 7.0, 8.0};
    std::array<blitzar_core::Scalar, ParticleCount> position_z{9.0, 1.0, 2.0, 3.0};
    std::array<blitzar_core::Scalar, ParticleCount> velocity_x{0.1, 0.2, 0.3, 0.4};
    std::array<blitzar_core::Scalar, ParticleCount> velocity_y{0.5, 0.6, 0.7, 0.8};
    std::array<blitzar_core::Scalar, ParticleCount> velocity_z{0.9, 1.1, 1.2, 1.3};
    std::array<blitzar_core::Scalar, ParticleCount> mass{1.0, 2.0, 3.0, 4.0};
    std::array<std::uint32_t, ParticleCount> source_rank{0, 0, 1, 1};
};

[[nodiscard]] blitzar_io::SnapshotV2State MakeState(V2Storage& storage) noexcept
{
    blitzar_io::SnapshotV2State state{};

    state.particles = {std::span<const std::uint64_t>(storage.ids),
        std::span<const blitzar_core::Scalar>(storage.position_x),
        std::span<const blitzar_core::Scalar>(storage.position_y),
        std::span<const blitzar_core::Scalar>(storage.position_z),
        std::span<const blitzar_core::Scalar>(storage.velocity_x),
        std::span<const blitzar_core::Scalar>(storage.velocity_y),
        std::span<const blitzar_core::Scalar>(storage.velocity_z),
        std::span<const blitzar_core::Scalar>(storage.mass)};

    state.integrator = {7, 3.5, 0.5, blitzar_io::SnapshotV2IntegratorKind::LeapfrogKdk};
    state.rng = {42};
    state.units = {{1.0, 2.0, 3.0}};
    state.math = {blitzar_core::ExecutionSettings::Strict(),
        blitzar_io::SnapshotV2CompensatorKind::DirectPlain};

    state.backend = {blitzar_io::SnapshotV2BackendKind::Cpu,
        blitzar_io::SnapshotV2DeviceKind::HostCpu, blitzar_io::SnapshotV2PrecisionKind::Float64};

    state.domain = {{0.0, 0.0, 0.0}, {4.0, 8.0, 3.0}, 2U, 1U};
    state.ownership = {blitzar_io::SnapshotV2OwnershipKind::PositionPartition, 2U,
        std::span<const std::uint32_t>(storage.source_rank)};

    return state;
}

[[nodiscard]] blitzar_io::SnapshotV2Target MakeTarget(V2Storage& storage) noexcept
{
    return {std::span<std::uint64_t>(storage.ids),
        std::span<blitzar_core::Scalar>(storage.position_x),
        std::span<blitzar_core::Scalar>(storage.position_y),
        std::span<blitzar_core::Scalar>(storage.position_z),
        std::span<blitzar_core::Scalar>(storage.velocity_x),
        std::span<blitzar_core::Scalar>(storage.velocity_y),
        std::span<blitzar_core::Scalar>(storage.velocity_z),
        std::span<blitzar_core::Scalar>(storage.mass),
        std::span<std::uint32_t>(storage.source_rank), {}, {}, {}, {}, {}, {}, {}, {}};
}

[[nodiscard]] bool SameState(const V2Storage& first, const V2Storage& second) noexcept
{
    return first.ids == second.ids && first.position_x == second.position_x &&
           first.position_y == second.position_y && first.position_z == second.position_z &&
           first.velocity_x == second.velocity_x && first.velocity_y == second.velocity_y &&
           first.velocity_z == second.velocity_z && first.mass == second.mass &&
           first.source_rank == second.source_rank;
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

int CheckRoundTrip(const V2Storage& input, const std::filesystem::path& first_path,
    const std::filesystem::path& second_path)
{
    const blitzar_io::SnapshotV2State state = MakeState(const_cast<V2Storage&>(input));
    const blitzar_io::SnapshotV2Writer writer(ParticleCount);

    BLITZAR_CHECK(writer.Write(first_path, state) == BLITZAR_STATUS_OK);
    BLITZAR_CHECK(writer.Write(second_path, state) == BLITZAR_STATUS_OK);
    BLITZAR_CHECK(ReadBytes(first_path) == ReadBytes(second_path));

    V2Storage output{};
    blitzar_io::SnapshotV2Reader reader(ParticleCount);
    blitzar_io::SnapshotV2Target target = MakeTarget(output);

    BLITZAR_CHECK(reader.Read(first_path, target) == BLITZAR_STATUS_OK);
    BLITZAR_CHECK(SameState(input, output));

    return 0;
}

int CheckByteParity(const V2Storage& input, const std::filesystem::path& first_path,
    const std::filesystem::path& second_path)
{
    const blitzar_io::SnapshotV2Writer writer(ParticleCount);
    blitzar_io::SnapshotV2Reader reader(ParticleCount);

    BLITZAR_CHECK(
        writer.Write(first_path, MakeState(const_cast<V2Storage&>(input))) == BLITZAR_STATUS_OK);

    V2Storage decoded{};
    blitzar_io::SnapshotV2Target target = MakeTarget(decoded);

    BLITZAR_CHECK(reader.Read(first_path, target) == BLITZAR_STATUS_OK);
    BLITZAR_CHECK(writer.Write(second_path, MakeState(decoded)) == BLITZAR_STATUS_OK);
    BLITZAR_CHECK(ReadBytes(first_path) == ReadBytes(second_path));

    return 0;
}

int CheckRejections(const V2Storage& input, const std::filesystem::path& probe_path)
{
    const blitzar_io::SnapshotV2State state = MakeState(const_cast<V2Storage&>(input));
    const blitzar_io::SnapshotV2Writer writer(ParticleCount);

    BLITZAR_CHECK(writer.Write(probe_path, state) == BLITZAR_STATUS_OK);

    const std::vector<std::uint8_t> valid_bytes = ReadBytes(probe_path);

    BLITZAR_CHECK(!valid_bytes.empty());

    V2Storage target_storage{};
    blitzar_io::SnapshotV2Reader reader(ParticleCount);
    blitzar_io::SnapshotV2Target read_target = MakeTarget(target_storage);

    BLITZAR_CHECK(reader.Read(probe_path, read_target) == BLITZAR_STATUS_OK);

    const std::array<std::uint64_t, ParticleCount> saved_ids = target_storage.ids;

    std::vector<std::uint8_t> corrupted = valid_bytes;

    corrupted.back() ^= 1U;

    BLITZAR_CHECK(WriteBytes(probe_path, corrupted));
    BLITZAR_CHECK(reader.Read(probe_path, read_target) == BLITZAR_STATUS_INVALID_ARGUMENT);

    std::vector<std::uint8_t> truncated = valid_bytes;

    truncated.pop_back();

    BLITZAR_CHECK(WriteBytes(probe_path, truncated));
    BLITZAR_CHECK(reader.Read(probe_path, read_target) == BLITZAR_STATUS_INVALID_ARGUMENT);

    std::vector<std::uint8_t> unknown_tag = valid_bytes;

    unknown_tag[8] = 99U;

    BLITZAR_CHECK(WriteBytes(probe_path, unknown_tag));
    BLITZAR_CHECK(reader.Read(probe_path, read_target) == BLITZAR_STATUS_UNSUPPORTED);

    std::vector<std::uint8_t> incompatible_version = valid_bytes;

    incompatible_version[4] = 7U;

    BLITZAR_CHECK(WriteBytes(probe_path, incompatible_version));
    BLITZAR_CHECK(reader.Read(probe_path, read_target) == BLITZAR_STATUS_UNSUPPORTED);

    BLITZAR_CHECK(target_storage.ids == saved_ids);

    return 0;
}

int CheckRepartition()
{
    V2Storage storage{};

    const blitzar_io::SnapshotV2State state = MakeState(storage);
    const blitzar_io::SnapshotRepartition repartition;

    BLITZAR_CHECK(repartition.RankCount(1) == 1U);
    BLITZAR_CHECK(repartition.RankCount(8) == 8U);
    BLITZAR_CHECK(repartition.RankCount(9) == 9U);

    const std::array<blitzar_core::Vector3, ParticleCount> positions{
        {{1.0, 1.0, 1.0}, {2.0, 3.0, 1.0}, {3.0, 5.0, 2.0}, {4.0, 7.0, 3.0}}};

    std::array<std::uint32_t, ParticleCount> first_rank{};
    std::array<std::uint32_t, ParticleCount> second_rank{};
    std::array<std::uint64_t, 9> first_order{};
    std::array<std::uint64_t, 9> second_order{};

    const blitzar_io::SnapshotRepartitionRequest request{state.domain, 3U};

    BLITZAR_CHECK(
        repartition.Assign(request, storage.ids, std::span<const blitzar_core::Vector3>(positions),
            {std::span<std::uint32_t>(first_rank), std::span<std::uint64_t>(first_order)}) ==
        BLITZAR_STATUS_OK);

    BLITZAR_CHECK(
        repartition.Assign(request, storage.ids, std::span<const blitzar_core::Vector3>(positions),
            {std::span<std::uint32_t>(second_rank), std::span<std::uint64_t>(second_order)}) ==
        BLITZAR_STATUS_OK);

    BLITZAR_CHECK(first_rank == second_rank);
    BLITZAR_CHECK(first_order == second_order);

    std::uint64_t total = 0;

    for (const std::uint64_t count : first_order) {
        total += count;
    }

    BLITZAR_CHECK(total == ParticleCount);

    for (const std::uint32_t rank : first_rank) {
        BLITZAR_CHECK(rank < 3U);
    }

    return 0;
}

int CheckIdentity()
{
    blitzar_io::MetadataExecution fast{};

    fast.mode = blitzar_core::ExecutionMode::Fast;
    fast.cpu = blitzar_core::BackendExecutionPolicy{
        blitzar_core::FmaPolicy::Hardware, blitzar_core::ReductionPolicy::BackendDefined};

    fast.hip = fast.cpu;
    fast.mpi = fast.cpu;
    fast.backend = "runtime-selected";
    fast.device = "runtime-selected";
    fast.compensator = "backend-defined;diagnostics-neumaier-v1";
    fast.bitwise_reproducible = false;

    BLITZAR_CHECK(fast.Validate() == BLITZAR_STATUS_OK);
    BLITZAR_CHECK(!fast.IsResolvedIdentity());
    BLITZAR_CHECK(fast.ValidateCompleted() == BLITZAR_STATUS_INVALID_ARGUMENT);

    BLITZAR_CHECK(
        fast.ResolveIdentity("hip", "runtime-selected") == BLITZAR_STATUS_INVALID_ARGUMENT);

    BLITZAR_CHECK(fast.ResolveIdentity("cpu", "host-cpu") == BLITZAR_STATUS_OK);
    BLITZAR_CHECK(fast.IsResolvedIdentity());
    BLITZAR_CHECK(fast.ValidateCompleted() == BLITZAR_STATUS_OK);

    blitzar_io::MetadataExecution strict{};

    strict.backend = "cpu";
    strict.device = "host-cpu";
    strict.bitwise_reproducible = true;

    BLITZAR_CHECK(strict.IsResolvedIdentity());
    BLITZAR_CHECK(strict.ValidateCompleted() == BLITZAR_STATUS_OK);

    return 0;
}

} // namespace

int main(int argc, char** argv)
{
    const std::string_view mode = argc > 1 ? std::string_view(argv[1]) : std::string_view("all");
    const std::filesystem::path directory = std::filesystem::temp_directory_path();
    const std::filesystem::path first_path = directory / "blitzar-snapshot-v2-a.bin";
    const std::filesystem::path second_path = directory / "blitzar-snapshot-v2-b.bin";
    const std::filesystem::path probe_path = directory / "blitzar-snapshot-v2-probe.bin";

    std::error_code cleanup_error;

    std::filesystem::remove(first_path, cleanup_error);
    std::filesystem::remove(second_path, cleanup_error);
    std::filesystem::remove(probe_path, cleanup_error);

    V2Storage input{};

    const auto selected = [&mode](std::string_view name) { return mode == "all" || mode == name; };

    if (selected("roundtrip")) {
        BLITZAR_CHECK(CheckRoundTrip(input, first_path, second_path) == 0);
    }

    if (selected("parity")) {
        BLITZAR_CHECK(CheckByteParity(input, first_path, second_path) == 0);
    }

    if (selected("reject")) {
        BLITZAR_CHECK(CheckRejections(input, probe_path) == 0);
    }

    if (selected("repartition")) {
        BLITZAR_CHECK(CheckRepartition() == 0);
    }

    if (selected("identity")) {
        BLITZAR_CHECK(CheckIdentity() == 0);
    }

    std::filesystem::remove(first_path, cleanup_error);
    std::filesystem::remove(second_path, cleanup_error);
    std::filesystem::remove(probe_path, cleanup_error);

    return 0;
}
