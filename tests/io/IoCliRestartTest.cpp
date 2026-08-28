#include "BlitzarRestart.hpp"
#include "BlitzarRun.hpp"
#include "fixtures/FixtureCheck.hpp"
#include "io/snapshot/SnapshotReader.hpp"
#include "simulation/config/SimConfigFile.hpp"
#include "simulation/config/SimConfigRun.hpp"
#include "simulation/initialization/SimConfigState.hpp"

#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

constexpr std::string_view FullConfig =
    R"(simulation(particle_count=4, dt=0.1, solver=direct, integrator=leapfrog_kdk)
gravity(gravitational_constant=1.0, softening=0.01)
units(length_scale=1.0, mass_scale=1.0, time_scale=1.0)
generation(seed=42, deterministic=true)
run(steps=6)
output(directory="output", every_steps=2, write_initial=false, write_final=true)
)";

constexpr std::string_view SplitConfig =
    R"(simulation(particle_count=4, dt=0.1, solver=direct, integrator=leapfrog_kdk)
gravity(gravitational_constant=1.0, softening=0.01)
units(length_scale=1.0, mass_scale=1.0, time_scale=1.0)
generation(seed=42, deterministic=true)
run(steps=1)
output(directory="output", every_steps=2, write_initial=false, write_final=true)
)";

constexpr std::string_view RestartConfig =
    R"(simulation(particle_count=4, dt=0.1, solver=direct, integrator=leapfrog_kdk)
gravity(gravitational_constant=1.0, softening=0.01)
units(length_scale=1.0, mass_scale=1.0, time_scale=1.0)
generation(seed=42, deterministic=true)
run(steps=6)
output(directory="output", every_steps=2, write_initial=false, write_final=true)
restart(directory="../split/output", step=1)
)";

struct CapturedOutput final {
    int exit_code{};
    std::string standard_output;
    std::string standard_error;
};

void RemoveTree(const std::filesystem::path& path)
{
    std::error_code error;

    std::filesystem::remove_all(path, error);
}

bool WriteConfig(const std::filesystem::path& path, std::string_view source)
{
    std::ofstream output(path, std::ios::binary | std::ios::trunc);

    if (!output.is_open()) {
        return false;
    }

    output << source;

    return static_cast<bool>(output);
}

CapturedOutput RunCaptured(const std::filesystem::path& path)
{
    std::ostringstream standard_output;
    std::ostringstream standard_error;
    const int exit_code = blitzar_cli::RunConfig(path, {standard_output, standard_error});

    return {exit_code, standard_output.str(), standard_error.str()};
}

std::vector<std::uint8_t> ReadBytes(const std::filesystem::path& path)
{
    std::ifstream input(path, std::ios::binary);

    if (!input.is_open()) {
        return {};
    }

    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

bool WriteBytes(const std::filesystem::path& path, std::span<const std::uint8_t> bytes)
{
    std::ofstream output(path, std::ios::binary | std::ios::trunc);

    if (!output.is_open()) {
        return false;
    }

    output.write(
        reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));

    return static_cast<bool>(output);
}

int CheckSuccessfulRestart(const std::filesystem::path& base, std::filesystem::path& source_state,
    std::vector<std::uint8_t>& valid_bytes)
{
    const std::filesystem::path full_directory = base / "full";
    const std::filesystem::path split_directory = base / "split";
    const std::filesystem::path restart_directory = base / "restart";

    RemoveTree(base);
    std::filesystem::create_directories(full_directory);
    std::filesystem::create_directories(split_directory);
    std::filesystem::create_directories(restart_directory);

    BLITZAR_CHECK(WriteConfig(full_directory / "run.ini", FullConfig));
    BLITZAR_CHECK(WriteConfig(split_directory / "run.ini", SplitConfig));
    BLITZAR_CHECK(WriteConfig(restart_directory / "run.ini", RestartConfig));
    BLITZAR_CHECK(RunCaptured(full_directory / "run.ini").exit_code == 0);
    BLITZAR_CHECK(RunCaptured(split_directory / "run.ini").exit_code == 0);
    BLITZAR_CHECK(RunCaptured(restart_directory / "run.ini").exit_code == 0);

    const std::filesystem::path full_state =
        full_directory / "output" / "states" / "state-00000006.bin";

    source_state = split_directory / "output" / "states" / "state-00000001.bin";

    const std::filesystem::path restart_state =
        restart_directory / "output" / "states" / "state-00000006.bin";

    valid_bytes = ReadBytes(source_state);

    BLITZAR_CHECK(!valid_bytes.empty());
    BLITZAR_CHECK(ReadBytes(full_state) == ReadBytes(restart_state));

    return 0;
}

int CheckCorruptionAndTruncation(const std::filesystem::path& base,
    const std::filesystem::path& source_state, std::span<const std::uint8_t> valid_bytes)
{
    std::vector<std::uint8_t> corrupted(valid_bytes.begin(), valid_bytes.end());

    corrupted.back() ^= 1U;

    const std::filesystem::path corrupted_directory = base / "corrupted";

    std::filesystem::create_directories(corrupted_directory);

    BLITZAR_CHECK(WriteConfig(corrupted_directory / "run.ini", RestartConfig));
    BLITZAR_CHECK(WriteBytes(source_state, corrupted));

    const CapturedOutput corrupt_result = RunCaptured(corrupted_directory / "run.ini");

    BLITZAR_CHECK(corrupt_result.exit_code == 3);
    BLITZAR_CHECK(corrupt_result.standard_output.empty());
    BLITZAR_CHECK(!std::filesystem::exists(corrupted_directory / "output"));
    BLITZAR_CHECK(WriteBytes(source_state, valid_bytes));

    std::vector<std::uint8_t> truncated(valid_bytes.begin(), valid_bytes.end());

    truncated.pop_back();

    const std::filesystem::path truncated_directory = base / "truncated";

    std::filesystem::create_directories(truncated_directory);

    BLITZAR_CHECK(WriteConfig(truncated_directory / "run.ini", RestartConfig));
    BLITZAR_CHECK(WriteBytes(source_state, truncated));

    const CapturedOutput truncated_result = RunCaptured(truncated_directory / "run.ini");

    BLITZAR_CHECK(truncated_result.exit_code == 3);
    BLITZAR_CHECK(truncated_result.standard_output.empty());
    BLITZAR_CHECK(!std::filesystem::exists(truncated_directory / "output"));
    BLITZAR_CHECK(WriteBytes(source_state, valid_bytes));

    return 0;
}

int CheckCompatibility(const std::filesystem::path& base, const std::filesystem::path& source_state,
    std::span<const std::uint8_t> valid_bytes)
{
    const std::array<std::pair<std::string_view, std::string_view>, 4> changes{
        std::pair{"solver", "solver=barnes_hut"}, std::pair{"units", "length_scale=2.0"},
        std::pair{"count", "particle_count=3"}, std::pair{"integrator", "integrator=euler"}};

    for (const auto& [name, replacement] : changes) {
        std::string config(RestartConfig);
        const std::size_t position = name == "solver"  ? config.find("solver=direct")
                                     : name == "units" ? config.find("length_scale=1.0")
                                     : name == "count" ? config.find("particle_count=4")
                                                       : config.find("integrator=leapfrog_kdk");

        BLITZAR_CHECK(position != std::string::npos);

        const std::size_t length = name == "solver"  ? std::string_view{"solver=direct"}.size()
                                   : name == "units" ? std::string_view{"length_scale=1.0"}.size()
                                   : name == "count"
                                       ? std::string_view{"particle_count=4"}.size()
                                       : std::string_view{"integrator=leapfrog_kdk"}.size();

        config.replace(position, length, replacement);

        const std::filesystem::path directory = base / std::string(name);

        std::filesystem::create_directories(directory);

        BLITZAR_CHECK(WriteConfig(directory / "run.ini", config));

        const CapturedOutput result = RunCaptured(directory / "run.ini");

        BLITZAR_CHECK(result.exit_code == 3);
        BLITZAR_CHECK(result.standard_output.empty());
        BLITZAR_CHECK(!std::filesystem::exists(directory / "output"));
    }

    std::vector<std::uint8_t> incompatible(valid_bytes.begin(), valid_bytes.end());

    incompatible[6] = 4U;

    const std::filesystem::path scalar_directory = base / "scalar";

    std::filesystem::create_directories(scalar_directory);

    BLITZAR_CHECK(WriteConfig(scalar_directory / "run.ini", RestartConfig));
    BLITZAR_CHECK(WriteBytes(source_state, incompatible));

    const CapturedOutput scalar_result = RunCaptured(scalar_directory / "run.ini");

    BLITZAR_CHECK(scalar_result.exit_code == 3);
    BLITZAR_CHECK(scalar_result.standard_output.empty());
    BLITZAR_CHECK(!std::filesystem::exists(scalar_directory / "output"));
    BLITZAR_CHECK(WriteBytes(source_state, valid_bytes));

    return 0;
}

int CheckTransactionalFailure(const std::filesystem::path& base,
    const std::filesystem::path& source_state, std::span<const std::uint8_t> valid_bytes)
{
    const std::filesystem::path config_path = base / "transaction" / "run.ini";

    std::filesystem::create_directories(config_path.parent_path());

    BLITZAR_CHECK(WriteConfig(config_path, RestartConfig));

    blitzar_sim::SimConfigFile source;

    BLITZAR_CHECK(blitzar_sim::LoadConfig(config_path, source) == BLITZAR_STATUS_OK);

    blitzar_sim::SimConfigRun config;

    BLITZAR_CHECK(blitzar_sim::BuildRunConfig(source, config_path.parent_path(), config) ==
                  BLITZAR_STATUS_OK);

    blitzar_sim::SimConfigState state;

    BLITZAR_CHECK(blitzar_sim::BuildState(config, state) == BLITZAR_STATUS_OK);

    state.position_x[0] = -123.0;

    const double sentinel = state.position_x[0];

    std::vector<std::uint8_t> truncated(valid_bytes.begin(), valid_bytes.end());

    truncated.pop_back();

    BLITZAR_CHECK(WriteBytes(source_state, truncated));
    BLITZAR_CHECK(blitzar_cli::LoadRestartState(config, state) == BLITZAR_STATUS_INVALID_ARGUMENT);
    BLITZAR_CHECK(state.position_x[0] == sentinel);
    BLITZAR_CHECK(config.restart.time == 0.0);
    BLITZAR_CHECK(WriteBytes(source_state, valid_bytes));

    return 0;
}

} // namespace

int main()
{
    const std::filesystem::path base =
        std::filesystem::temp_directory_path() / "blitzar-cli-restart-647";

    std::filesystem::path source_state;
    std::vector<std::uint8_t> valid_bytes;

    BLITZAR_CHECK(CheckSuccessfulRestart(base, source_state, valid_bytes) == 0);
    BLITZAR_CHECK(CheckCorruptionAndTruncation(base, source_state, valid_bytes) == 0);
    BLITZAR_CHECK(CheckCompatibility(base, source_state, valid_bytes) == 0);
    BLITZAR_CHECK(CheckTransactionalFailure(base, source_state, valid_bytes) == 0);

    RemoveTree(base);

    return 0;
}
