#include "BlitzarRun.hpp"
#include "core/CoreSnapshot.hpp"
#include "fixtures/FixtureCheck.hpp"
#include "io/snapshot/SnapshotReader.hpp"

#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <span>
#include <sstream>
#include <string>
#include <string_view>

namespace {

constexpr std::string_view InitialPeriodicFinalConfig =
    R"(simulation(particle_count=4, dt=0.01, solver=direct, integrator=leapfrog_kdk)
gravity(gravitational_constant=1.0, softening=0.01)
units(length_scale=1.0, mass_scale=1.0, time_scale=1.0)
generation(seed=42, deterministic=true)
run(steps=4)
output(directory="output", every_steps=2, write_initial=true, write_final=true)
)";

constexpr std::string_view PeriodicOnlyConfig =
    R"(simulation(particle_count=4, dt=0.01, solver=direct, integrator=leapfrog_kdk)
gravity(gravitational_constant=1.0, softening=0.01)
units(length_scale=1.0, mass_scale=1.0, time_scale=1.0)
generation(seed=42, deterministic=true)
run(steps=4)
output(directory="output", every_steps=2, write_initial=false, write_final=false)
)";

constexpr std::string_view FinalOnlyConfig =
    R"(simulation(particle_count=4, dt=0.01, solver=direct, integrator=leapfrog_kdk)
gravity(gravitational_constant=1.0, softening=0.01)
units(length_scale=1.0, mass_scale=1.0, time_scale=1.0)
generation(seed=42, deterministic=true)
run(steps=3)
output(directory="output", every_steps=10, write_initial=false, write_final=true)
)";

constexpr std::string_view NoOutputConfig =
    R"(simulation(particle_count=4, dt=0.01, solver=direct, integrator=leapfrog_kdk)
gravity(gravitational_constant=1.0, softening=0.01)
units(length_scale=1.0, mass_scale=1.0, time_scale=1.0)
generation(seed=42, deterministic=true)
run(steps=2)
)";

struct ExpectedFrame final {
    std::uint64_t step{};
    double time{};
    std::string_view name;
};

void RemoveTree(const std::filesystem::path& path)
{
    std::error_code error;

    std::filesystem::remove_all(path, error);
}

int WriteConfig(const std::filesystem::path& path, std::string_view source)
{
    std::ofstream output(path, std::ios::binary | std::ios::trunc);

    BLITZAR_CHECK(output.is_open());

    output << source;

    BLITZAR_CHECK(static_cast<bool>(output));

    return 0;
}

[[nodiscard]] std::string ReadText(const std::filesystem::path& path)
{
    std::ifstream input(path, std::ios::binary);

    if (!input.is_open()) {
        return {};
    }

    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

int CheckSnapshot(const std::filesystem::path& path, const ExpectedFrame& expected)
{
    std::array<std::uint64_t, 4> ids{};
    std::array<double, 4> position_x{};
    std::array<double, 4> position_y{};
    std::array<double, 4> position_z{};
    std::array<double, 4> velocity_x{};
    std::array<double, 4> velocity_y{};
    std::array<double, 4> velocity_z{};
    std::array<double, 4> mass{};
    const blitzar_core::SnapshotMutablePayloadView payload{std::span<std::uint64_t>(ids),
        std::span<double>(position_x), std::span<double>(position_y), std::span<double>(position_z),
        std::span<double>(velocity_x), std::span<double>(velocity_y), std::span<double>(velocity_z),
        std::span<double>(mass)};

    blitzar_core::SnapshotHeader header{};
    blitzar_io::SnapshotReader reader(4);

    BLITZAR_CHECK(reader.Read(path, header, payload) == BLITZAR_STATUS_OK);
    BLITZAR_CHECK(header.particle_count == 4U);
    BLITZAR_CHECK(header.step == expected.step);
    BLITZAR_CHECK(header.time == expected.time);

    for (std::size_t index = 0; index < ids.size(); ++index) {
        BLITZAR_CHECK(ids[index] == index);
        BLITZAR_CHECK(mass[index] > 0.0);
    }

    return 0;
}

int CheckOutputTree(const std::filesystem::path& root, std::span<const ExpectedFrame> expected)
{
    BLITZAR_CHECK(std::filesystem::is_directory(root));
    BLITZAR_CHECK(std::filesystem::is_directory(root / "states"));
    BLITZAR_CHECK(std::filesystem::is_directory(root / "diagnostics"));
    BLITZAR_CHECK(std::filesystem::is_regular_file(root / "manifest.json"));
    BLITZAR_CHECK(!std::filesystem::exists(root / "manifest.json.tmp"));

    std::size_t state_file_count = 0U;

    for (const std::filesystem::directory_entry& entry :
        std::filesystem::directory_iterator(root / "states")) {
        BLITZAR_CHECK(entry.is_regular_file());
        BLITZAR_CHECK(entry.path().extension() == ".bin");
        BLITZAR_CHECK(!std::filesystem::exists(entry.path().string() + ".tmp"));

        ++state_file_count;
    }

    BLITZAR_CHECK(state_file_count == expected.size());

    std::size_t diagnostic_file_count = 0U;

    for (const std::filesystem::directory_entry& entry :
        std::filesystem::directory_iterator(root / "diagnostics")) {
        BLITZAR_CHECK(entry.is_regular_file());

        ++diagnostic_file_count;
    }

    BLITZAR_CHECK(diagnostic_file_count == 0U);

    const std::string manifest = ReadText(root / "manifest.json");
    std::size_t previous_position = 0U;

    for (const ExpectedFrame& frame : expected) {
        const std::filesystem::path state_path = root / "states" / frame.name;

        BLITZAR_CHECK(std::filesystem::is_regular_file(state_path));
        BLITZAR_CHECK(CheckSnapshot(state_path, frame) == 0);

        const std::size_t position = manifest.find(frame.name);

        BLITZAR_CHECK(position != std::string::npos);
        BLITZAR_CHECK(position >= previous_position);

        previous_position = position;
    }

    BLITZAR_CHECK(manifest.find("\"completed_output_count\": " + std::to_string(expected.size())) !=
                  std::string::npos);

    return 0;
}

int RunCase(const std::filesystem::path& base, std::string_view source,
    std::span<const ExpectedFrame> expected, bool check_rerun)
{
    RemoveTree(base);
    std::filesystem::create_directories(base);

    const std::filesystem::path config = base / "run.ini";

    BLITZAR_CHECK(WriteConfig(config, source) == 0);
    BLITZAR_CHECK(blitzar_cli::RunConfig(config) == 0);
    BLITZAR_CHECK(CheckOutputTree(base / "output", expected) == 0);

    if (check_rerun) {
        BLITZAR_CHECK(blitzar_cli::RunConfig(config) == 5);
        BLITZAR_CHECK(CheckOutputTree(base / "output", expected) == 0);
    }

    return 0;
}

int CheckTiming(const std::filesystem::path& base, std::string_view source,
    std::uint64_t expected_checkpoint_count)
{
    RemoveTree(base);
    std::filesystem::create_directories(base);

    const std::filesystem::path config = base / "run.ini";

    BLITZAR_CHECK(WriteConfig(config, source) == 0);

    std::ostringstream standard_output;
    std::ostringstream standard_error;
    blitzar_cli::BlitzarRunTiming timing;

    BLITZAR_CHECK(blitzar_cli::RunConfig(config, {standard_output, standard_error}, timing) == 0);

    BLITZAR_CHECK(standard_error.str().empty());
    BLITZAR_CHECK(timing.physics_elapsed_ns > 0U);
    BLITZAR_CHECK(timing.output_checkpoint_count == expected_checkpoint_count);

    if (expected_checkpoint_count == 0U) {
        BLITZAR_CHECK(timing.output_elapsed_ns == 0U);
    }
    else {
        BLITZAR_CHECK(timing.output_elapsed_ns > 0U);
    }

    RemoveTree(base);

    return 0;
}

} // namespace

int main()
{
    const std::filesystem::path base =
        std::filesystem::temp_directory_path() / "blitzar-cli-output-645";

    constexpr std::array<ExpectedFrame, 3> initial_periodic_final{
        ExpectedFrame{0, 0.0, "state-00000000.bin"}, ExpectedFrame{2, 0.02, "state-00000002.bin"},
        ExpectedFrame{4, 0.04, "state-00000004.bin"}};

    constexpr std::array<ExpectedFrame, 2> periodic_only{
        ExpectedFrame{2, 0.02, "state-00000002.bin"}, ExpectedFrame{4, 0.04, "state-00000004.bin"}};

    constexpr std::array<ExpectedFrame, 1> final_only{ExpectedFrame{3, 0.03, "state-00000003.bin"}};

    BLITZAR_CHECK(RunCase(base / "initial-periodic-final", InitialPeriodicFinalConfig,
                      initial_periodic_final, true) == 0);

    BLITZAR_CHECK(RunCase(base / "periodic-only", PeriodicOnlyConfig, periodic_only, false) == 0);
    BLITZAR_CHECK(RunCase(base / "final-only", FinalOnlyConfig, final_only, false) == 0);
    BLITZAR_CHECK(CheckTiming(base / "timing-output", InitialPeriodicFinalConfig, 3U) == 0);
    BLITZAR_CHECK(CheckTiming(base / "timing-none", NoOutputConfig, 0U) == 0);

    RemoveTree(base);

    return 0;
}
