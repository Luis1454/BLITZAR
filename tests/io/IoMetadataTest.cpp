#include "core/CoreSnapshot.hpp"
#include "fixtures/FixtureCheck.hpp"
#include "io/metadata/MetadataReader.hpp"
#include "io/metadata/MetadataRun.hpp"

#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace {

struct SnapshotStorage final {
    std::array<std::uint64_t, 2> ids{0, 1};
    std::array<double, 2> position_x{1.0, 2.0};
    std::array<double, 2> position_y{3.0, 4.0};
    std::array<double, 2> position_z{5.0, 6.0};
    std::array<double, 2> velocity_x{0.1, 0.2};
    std::array<double, 2> velocity_y{0.3, 0.4};
    std::array<double, 2> velocity_z{0.5, 0.6};
    std::array<double, 2> mass{1.0, 2.0};
};

[[nodiscard]] blitzar_io::MetadataRunInfo MakeInfo()
{
    blitzar_io::MetadataRunInfo info;

    info.product_version = "1.0.0";
    info.plan_version = "1.0.31";
    info.configuration.simulation = {
        2, 2, 0.01, BLITZAR_SOLVER_DIRECT, BLITZAR_INTEGRATOR_LEAPFROG_KDK};

    info.configuration.gravity = {1.0, 0.01};
    info.configuration.units = {1.0, 1.0, 1.0};
    info.configuration.barnes_hut = {0.5, 2, 17, 8, 32};
    info.configuration.generation = {42, true};
    info.configuration.output = {true, 2, true, true};
    info.configuration.diagnostics = {true, 2, true, true, true};
    info.capabilities = {
        BLITZAR_SOLVER_MASK_DIRECT | BLITZAR_SOLVER_MASK_BARNES_HUT | BLITZAR_SOLVER_MASK_FMM,
        BLITZAR_SOLVER_MASK_PM | BLITZAR_SOLVER_MASK_TREEPM,
        BLITZAR_FEATURE_GRID | BLITZAR_FEATURE_HDF5, BLITZAR_BACKEND_MASK_CPU};

    return info;
}

[[nodiscard]] blitzar_core::SnapshotFrameView MakeFrame(
    const SnapshotStorage& storage, std::uint64_t step, double time) noexcept
{
    blitzar_core::SnapshotHeader header{};

    header.particle_count = storage.ids.size();
    header.step = step;
    header.time = time;

    const blitzar_core::SnapshotPayloadView payload{std::span<const std::uint64_t>(storage.ids),
        std::span<const double>(storage.position_x), std::span<const double>(storage.position_y),
        std::span<const double>(storage.position_z), std::span<const double>(storage.velocity_x),
        std::span<const double>(storage.velocity_y), std::span<const double>(storage.velocity_z),
        std::span<const double>(storage.mass)};

    return {header, payload};
}

[[nodiscard]] std::string ReadText(const std::filesystem::path& path)
{
    std::ifstream input(path);

    if (!input.is_open()) {
        return {};
    }

    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

void RemoveTree(const std::filesystem::path& path)
{
    std::error_code error;

    std::filesystem::remove_all(path, error);
}

int CheckPreparation(const std::filesystem::path& root)
{
    RemoveTree(root);

    blitzar_io::MetadataRun run(root, MakeInfo());

    BLITZAR_CHECK(!std::filesystem::exists(root));

    const blitzar_status preparation_status = run.Prepare();

    BLITZAR_CHECK(preparation_status == BLITZAR_STATUS_OK);
    BLITZAR_CHECK(std::filesystem::is_directory(run.Root()));
    BLITZAR_CHECK(std::filesystem::is_directory(run.StatesPath()));
    BLITZAR_CHECK(std::filesystem::is_directory(run.DiagnosticsPath()));
    BLITZAR_CHECK(std::filesystem::is_regular_file(run.ManifestPath()));
    BLITZAR_CHECK(run.CompletedOutputCount() == 0U);

    const std::string manifest = ReadText(run.ManifestPath());

    BLITZAR_CHECK(manifest.find("\"completed_output_count\": 0") != std::string::npos);
    BLITZAR_CHECK(manifest.find("\"product_version\": \"1.0.0\"") != std::string::npos);
    BLITZAR_CHECK(manifest.find("\"particle_count\": 2") != std::string::npos);
    BLITZAR_CHECK(manifest.find("\"solver\": 0") != std::string::npos);
    BLITZAR_CHECK(manifest.find("\"integrator\": 0") != std::string::npos);
    BLITZAR_CHECK(manifest.find("\"gravitational_constant\": 1") != std::string::npos);
    BLITZAR_CHECK(manifest.find("\"length_scale\": 1") != std::string::npos);
    BLITZAR_CHECK(manifest.find("\"seed\": 42") != std::string::npos);
    BLITZAR_CHECK(manifest.find("\"deterministic\": true") != std::string::npos);
    BLITZAR_CHECK(manifest.find("\"compiled_backend_mask\": 1") != std::string::npos);
    BLITZAR_CHECK(run.Prepare() == BLITZAR_STATUS_INVALID_ARGUMENT);

    return 0;
}

int CheckDeterministicManifest(
    const std::filesystem::path& first_root, const std::filesystem::path& second_root)
{
    RemoveTree(first_root);
    RemoveTree(second_root);

    blitzar_io::MetadataRun first(first_root, MakeInfo());
    blitzar_io::MetadataRun second(second_root, MakeInfo());

    BLITZAR_CHECK(first.Prepare() == BLITZAR_STATUS_OK);
    BLITZAR_CHECK(second.Prepare() == BLITZAR_STATUS_OK);
    BLITZAR_CHECK(ReadText(first.ManifestPath()) == ReadText(second.ManifestPath()));

    return 0;
}

int CheckPublication(const std::filesystem::path& root)
{
    RemoveTree(root);

    blitzar_io::MetadataRun run(root, MakeInfo());
    SnapshotStorage storage{};

    BLITZAR_CHECK(run.Prepare() == BLITZAR_STATUS_OK);
    BLITZAR_CHECK(run.PublishSnapshot(MakeFrame(storage, 0, 0.0)) == BLITZAR_STATUS_OK);
    BLITZAR_CHECK(std::filesystem::is_regular_file(run.StatePath(0)));
    BLITZAR_CHECK(!std::filesystem::exists(run.StatePath(0).string() + ".tmp"));
    BLITZAR_CHECK(!std::filesystem::exists(run.ManifestPath().string() + ".tmp"));

    BLITZAR_CHECK(run.PublishSnapshot(MakeFrame(storage, 2, 0.02)) == BLITZAR_STATUS_OK);
    BLITZAR_CHECK(run.CompletedOutputCount() == 2U);
    BLITZAR_CHECK(
        run.PublishSnapshot(MakeFrame(storage, 2, 0.02)) == BLITZAR_STATUS_INVALID_ARGUMENT);

    BLITZAR_CHECK(
        run.PublishSnapshot(MakeFrame(storage, 1, 0.01)) == BLITZAR_STATUS_INVALID_ARGUMENT);

    BLITZAR_CHECK(!std::filesystem::exists(run.StatePath(1)));

    blitzar_core::SnapshotFrameView invalid_frame = MakeFrame(storage, 1, 0.01);

    invalid_frame.payload.position_x = {};

    BLITZAR_CHECK(run.PublishSnapshot(invalid_frame) == BLITZAR_STATUS_INVALID_ARGUMENT);
    BLITZAR_CHECK(!std::filesystem::exists(run.StatePath(1)));

    const std::string manifest = ReadText(run.ManifestPath());
    const std::size_t first_output = manifest.find("state-00000000.bin");
    const std::size_t second_output = manifest.find("state-00000002.bin");

    BLITZAR_CHECK(manifest.find("\"completed_output_count\": 2") != std::string::npos);
    BLITZAR_CHECK(first_output != std::string::npos);
    BLITZAR_CHECK(second_output != std::string::npos);
    BLITZAR_CHECK(first_output < second_output);

    return 0;
}

int CheckRejectedRoots(const std::filesystem::path& root)
{
    RemoveTree(root);
    std::filesystem::create_directories(root);

    const std::filesystem::path sentinel = root / "sentinel";

    std::ofstream(sentinel) << "occupied";

    blitzar_io::MetadataRun occupied(root, MakeInfo());

    BLITZAR_CHECK(occupied.Prepare() == BLITZAR_STATUS_INVALID_ARGUMENT);
    BLITZAR_CHECK(!std::filesystem::exists(occupied.StatesPath()));
    BLITZAR_CHECK(std::filesystem::is_regular_file(sentinel));

    RemoveTree(root);
    std::ofstream(root) << "not a directory";

    blitzar_io::MetadataRun file_root(root, MakeInfo());

    BLITZAR_CHECK(file_root.Prepare() == BLITZAR_STATUS_INVALID_ARGUMENT);

    RemoveTree(root);

    blitzar_io::MetadataRunInfo distributed_info = MakeInfo();

    distributed_info.rank_count = 2;

    blitzar_io::MetadataRun distributed(root, std::move(distributed_info));

    BLITZAR_CHECK(distributed.Prepare() == BLITZAR_STATUS_OK);
    BLITZAR_CHECK(std::filesystem::is_directory(distributed.StatesPath()));
    BLITZAR_CHECK(distributed.StatePath(0).filename() == "state-00000000.rank-00000000.bin");

    BLITZAR_CHECK(blitzar_io::StateFileName(99999999) == "state-99999999.bin");
    BLITZAR_CHECK(blitzar_io::StateFileName(100000000).empty());
    BLITZAR_CHECK(blitzar_io::StateShardFileName(0, 1, blitzar_io::MetadataOutputFormat::Binary) ==
                  "state-00000000.rank-00000001.bin");

    return 0;
}

int CheckDistributedPublication(const std::filesystem::path& root)
{
    RemoveTree(root);

    blitzar_io::MetadataRunInfo root_info = MakeInfo();

    root_info.rank_count = 2;

    blitzar_io::MetadataRunInfo rank_info = root_info;

    rank_info.rank_index = 1;

    blitzar_io::MetadataRun root_run(root, std::move(root_info));
    blitzar_io::MetadataRun rank_run(root, std::move(rank_info));
    SnapshotStorage storage{};

    auto root_frame = MakeFrame(storage, 0, 0.0);

    root_frame.header.rank_count = 2;
    root_frame.header.distribution = blitzar_core::SnapshotDistribution::Sharded;
    root_frame.header.id_policy = blitzar_core::SnapshotIdPolicy::GlobalStable;

    auto rank_frame = root_frame;

    rank_frame.header.rank_index = 1;

    BLITZAR_CHECK(root_run.Prepare(true) == BLITZAR_STATUS_OK);
    BLITZAR_CHECK(rank_run.Prepare(false) == BLITZAR_STATUS_OK);
    BLITZAR_CHECK(root_run.PublishSnapshot(root_frame) == BLITZAR_STATUS_OK);
    BLITZAR_CHECK(rank_run.PublishSnapshot(rank_frame) == BLITZAR_STATUS_OK);
    BLITZAR_CHECK(root_run.CompletedOutputCount() == 0U);
    BLITZAR_CHECK(root_run.CommitDistributedSnapshot(0) == BLITZAR_STATUS_OK);
    BLITZAR_CHECK(root_run.CompletedOutputCount() == 1U);

    const std::string manifest = ReadText(root_run.ManifestPath());
    const std::size_t first_shard = manifest.find("state-00000000.rank-00000000.bin");
    const std::size_t second_shard = manifest.find("state-00000000.rank-00000001.bin");

    BLITZAR_CHECK(manifest.find("\"shards\": [") != std::string::npos);
    BLITZAR_CHECK(first_shard != std::string::npos);
    BLITZAR_CHECK(second_shard != std::string::npos);
    BLITZAR_CHECK(first_shard < second_shard);
    BLITZAR_CHECK(std::filesystem::is_regular_file(root_run.StatePath(0)));
    BLITZAR_CHECK(std::filesystem::is_regular_file(rank_run.StatePath(0)));

    blitzar_io::MetadataReader reader;
    blitzar_io::MetadataRunInfo parsed_info;
    std::vector<std::uint64_t> completed_steps;

    BLITZAR_CHECK(
        reader.Read(root_run.ManifestPath(), parsed_info, completed_steps) == BLITZAR_STATUS_OK);

    BLITZAR_CHECK(parsed_info.rank_count == 2U);
    BLITZAR_CHECK(parsed_info.rank_index == 0U);
    BLITZAR_CHECK(completed_steps.size() == 1U && completed_steps[0] == 0U);

    return 0;
}

int CheckFailedPublication(const std::filesystem::path& root)
{
    RemoveTree(root);

    blitzar_io::MetadataRun run(root, MakeInfo());
    SnapshotStorage storage{};

    BLITZAR_CHECK(run.Prepare() == BLITZAR_STATUS_OK);
    BLITZAR_CHECK(std::filesystem::create_directory(run.StatePath(0)));
    BLITZAR_CHECK(
        run.PublishSnapshot(MakeFrame(storage, 0, 0.0)) == BLITZAR_STATUS_INVALID_ARGUMENT);

    BLITZAR_CHECK(run.CompletedOutputCount() == 0U);
    BLITZAR_CHECK(
        ReadText(run.ManifestPath()).find("\"completed_output_count\": 0") != std::string::npos);

    BLITZAR_CHECK(!std::filesystem::exists(run.StatePath(0).string() + ".tmp"));

    return 0;
}

} // namespace

int main()
{
    const std::filesystem::path base =
        std::filesystem::temp_directory_path() / "blitzar-metadata-643";

    BLITZAR_CHECK(CheckPreparation(base / "preparation") == 0);
    BLITZAR_CHECK(
        CheckDeterministicManifest(base / "deterministic-a", base / "deterministic-b") == 0);

    BLITZAR_CHECK(CheckPublication(base / "publication") == 0);
    BLITZAR_CHECK(CheckRejectedRoots(base / "rejected") == 0);
    BLITZAR_CHECK(CheckFailedPublication(base / "failed-publication") == 0);
    BLITZAR_CHECK(CheckDistributedPublication(base / "distributed") == 0);

    RemoveTree(base);

    return 0;
}
