#include "fixtures/FixtureCheck.hpp"
#include "fixtures/FixtureProcess.hpp"
#include "fixtures/FixtureRestart.hpp"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <string_view>
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

bool WriteConfig(const std::filesystem::path& path, std::string_view source)
{
    std::ofstream output(path, std::ios::binary | std::ios::trunc);

    if (!output.is_open()) {
        return false;
    }

    output << source;

    return static_cast<bool>(output);
}

std::vector<std::uint8_t> ReadBytes(const std::filesystem::path& path)
{
    std::ifstream input(path, std::ios::binary);

    if (!input.is_open()) {
        return {};
    }

    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

bool CheckOutputs(const std::filesystem::path& root)
{
    const std::filesystem::path full_state =
        root / "full" / "output" / "states" / "state-00000006.bin";

    const std::filesystem::path restart_state =
        root / "restart" / "output" / "states" / "state-00000006.bin";

    return std::filesystem::is_regular_file(full_state) &&
           std::filesystem::is_regular_file(restart_state) &&
           ReadBytes(full_state) == ReadBytes(restart_state);
}

int RunChecks(const std::filesystem::path& executable, const std::filesystem::path& root)
{
    const std::filesystem::path full_directory = root / "full";
    const std::filesystem::path split_directory = root / "split";
    const std::filesystem::path restart_directory = root / "restart";

    BLITZAR_CHECK(blitzar_test::EnsureDirectory(full_directory));
    BLITZAR_CHECK(blitzar_test::EnsureDirectory(split_directory));
    BLITZAR_CHECK(blitzar_test::EnsureDirectory(restart_directory));
    BLITZAR_CHECK(WriteConfig(full_directory / "run.ini", FullConfig));
    BLITZAR_CHECK(WriteConfig(split_directory / "run.ini", SplitConfig));
    BLITZAR_CHECK(WriteConfig(restart_directory / "run.ini", RestartConfig));
    BLITZAR_CHECK(blitzar_test::RunProcess(executable, "--config", full_directory / "run.ini"));
    BLITZAR_CHECK(blitzar_test::RunProcess(executable, "--config", split_directory / "run.ini"));
    BLITZAR_CHECK(blitzar_test::RunProcess(executable, "--config", restart_directory / "run.ini"));

    BLITZAR_CHECK(CheckOutputs(root));

    return 0;
}

} // namespace

int main(int argc, char** argv)
{
    if (argc != 2) {
        return 1;
    }

    std::filesystem::path root;

    if (!blitzar_test::AcquireTestDirectory(root, "blitzar-cli-process-restart-666-")) {
        return 1;
    }

    const int result = RunChecks(argv[1], root);
    const bool cleanup_succeeded = blitzar_test::RemoveTree(root);

    return result == 0 && cleanup_succeeded ? 0 : 1;
}
