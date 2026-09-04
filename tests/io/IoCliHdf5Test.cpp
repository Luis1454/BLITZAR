#include "BlitzarRun.hpp"
#include "fixtures/FixtureCheck.hpp"
#include "fixtures/FixtureRestart.hpp"
#include "io/hdf5/Hdf5Reader.hpp"
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

constexpr std::string_view Hdf5Config =
    R"(simulation(particle_count=4, dt=0.1, solver=direct, integrator=leapfrog_kdk)
gravity(gravitational_constant=1.0, softening=0.01)
units(length_scale=1.0, mass_scale=1.0, time_scale=1.0)
generation(seed=42, deterministic=true)
run(steps=2)
output(directory="output", every_steps=1, write_initial=true, write_final=true, format=hdf5)
)";

constexpr std::string_view BinaryConfig =
    R"(simulation(particle_count=4, dt=0.1, solver=direct, integrator=leapfrog_kdk)
gravity(gravitational_constant=1.0, softening=0.01)
units(length_scale=1.0, mass_scale=1.0, time_scale=1.0)
generation(seed=42, deterministic=true)
run(steps=2)
output(directory="output", every_steps=1, write_initial=true, write_final=true)
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

[[nodiscard]] std::string ReadText(const std::filesystem::path& path)
{
    std::ifstream input(path, std::ios::binary);

    if (!input.is_open()) {
        return {};
    }

    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

int CheckHdf5State(const std::filesystem::path& path, std::uint64_t expected_step)
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
    blitzar_io::Hdf5Reader reader(4);

    BLITZAR_CHECK(reader.Read(path, header, payload) == BLITZAR_STATUS_OK);
    BLITZAR_CHECK(header.particle_count == 4U);
    BLITZAR_CHECK(header.step == expected_step);
    BLITZAR_CHECK(header.time == static_cast<double>(expected_step) * 0.1);

    for (std::size_t index = 0; index < ids.size(); ++index) {
        BLITZAR_CHECK(ids[index] == index);
        BLITZAR_CHECK(mass[index] > 0.0);
    }

    return 0;
}

int CheckBinaryState(const std::filesystem::path& path)
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
    BLITZAR_CHECK(header.step == 2U);
    BLITZAR_CHECK(header.time == 0.2);

    return 0;
}

int RunCaptured(const std::filesystem::path& path)
{
    std::ostringstream output;
    std::ostringstream error;

    return blitzar_cli::RunConfig(path, {output, error});
}

int CheckHdf5Selection(const std::filesystem::path& base)
{
    const std::filesystem::path directory = base / "hdf5";
    const std::filesystem::path config = directory / "run.ini";

    BLITZAR_CHECK(blitzar_test::EnsureDirectory(directory));
    BLITZAR_CHECK(WriteConfig(config, Hdf5Config));

    const int first_result = RunCaptured(config);

    if (!blitzar_io::Hdf5Reader::IsAvailable()) {
        BLITZAR_CHECK(first_result == 5);
        BLITZAR_CHECK(!std::filesystem::exists(directory / "output"));

        return 0;
    }

    BLITZAR_CHECK(first_result == 0);

    const std::filesystem::path output = directory / "output";
    const std::string manifest = ReadText(output / "manifest.json");

    BLITZAR_CHECK(manifest.find("\"format\": \"hdf5\"") != std::string::npos);
    BLITZAR_CHECK(manifest.find("state-00000000.h5") != std::string::npos);
    BLITZAR_CHECK(manifest.find("state-00000001.h5") != std::string::npos);
    BLITZAR_CHECK(manifest.find("state-00000002.h5") != std::string::npos);

    for (std::uint64_t step = 0; step <= 2U; ++step) {
        const std::filesystem::path state =
            output / "states" / ("state-0000000" + std::to_string(step) + ".h5");

        BLITZAR_CHECK(std::filesystem::is_regular_file(state));
        BLITZAR_CHECK(!std::filesystem::exists(state.string() + ".tmp"));
        BLITZAR_CHECK(CheckHdf5State(state, step) == 0);
    }

    BLITZAR_CHECK(RunCaptured(config) == 5);

    return 0;
}

int CheckBinaryFallback(const std::filesystem::path& base)
{
    const std::filesystem::path directory = base / "binary";
    const std::filesystem::path config = directory / "run.ini";

    BLITZAR_CHECK(blitzar_test::EnsureDirectory(directory));
    BLITZAR_CHECK(WriteConfig(config, BinaryConfig));
    BLITZAR_CHECK(RunCaptured(config) == 0);
    BLITZAR_CHECK(CheckBinaryState(directory / "output" / "states" / "state-00000002.bin") == 0);
    BLITZAR_CHECK(ReadText(directory / "output" / "manifest.json").find("\"format\": \"binary\"") !=
                  std::string::npos);

    return 0;
}

} // namespace

int main()
{
    std::filesystem::path base;

    BLITZAR_CHECK(blitzar_test::AcquireTestDirectory(base, "blitzar-cli-hdf5-684-"));
    BLITZAR_CHECK(CheckHdf5Selection(base) == 0);
    BLITZAR_CHECK(CheckBinaryFallback(base) == 0);
    BLITZAR_CHECK(blitzar_test::RemoveTree(base));

    return 0;
}
