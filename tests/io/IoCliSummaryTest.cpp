#include "BlitzarRun.hpp"
#include "BlitzarSummary.hpp"
#include "fixtures/FixtureCheck.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>

namespace {

constexpr std::string_view NoOutputConfig =
    R"(simulation(particle_count=4, dt=0.01, solver=direct, integrator=leapfrog_kdk)
gravity(gravitational_constant=1.0, softening=0.01)
units(length_scale=1.0, mass_scale=1.0, time_scale=1.0)
generation(seed=42, deterministic=true)
run(steps=2)
)";

constexpr std::string_view OutputConfig =
    R"(simulation(particle_count=4, dt=0.01, solver=direct, integrator=leapfrog_kdk)
gravity(gravitational_constant=1.0, softening=0.01)
units(length_scale=1.0, mass_scale=1.0, time_scale=1.0)
generation(seed=42, deterministic=true)
run(steps=2)
output(directory="output", every_steps=1, write_initial=true, write_final=false)
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

std::string ExpectedSummary(std::string_view output_path, std::size_t snapshot_count)
{
    std::ostringstream expected;

    expected << "{\"schema_version\":1,\"status\":0,\"requested_steps\":2,\"completed_steps\":2,"
                "\"particle_count\":4,\"solver\":0,\"snapshot_count\":"
             << snapshot_count << ",\"diagnostics_count\":0,\"output_path\":\"" << output_path
             << "\"}\n";

    return expected.str();
}

int CheckNoOutputSummary(const std::filesystem::path& base)
{
    RemoveTree(base);
    std::filesystem::create_directories(base);

    const std::filesystem::path config = base / "run.ini";

    BLITZAR_CHECK(WriteConfig(config, NoOutputConfig));

    const CapturedOutput first = RunCaptured(config);
    const CapturedOutput second = RunCaptured(config);
    const std::string expected = ExpectedSummary("", 0U);

    BLITZAR_CHECK(first.exit_code == 0);
    BLITZAR_CHECK(first.standard_error.empty());
    BLITZAR_CHECK(first.standard_output == expected);
    BLITZAR_CHECK(second.exit_code == 0);
    BLITZAR_CHECK(second.standard_error.empty());
    BLITZAR_CHECK(second.standard_output == expected);
    BLITZAR_CHECK(first.standard_output == second.standard_output);

    return 0;
}

int CheckOutputSummary(const std::filesystem::path& base)
{
    RemoveTree(base);
    std::filesystem::create_directories(base);

    const std::filesystem::path config = base / "run.ini";
    const std::filesystem::path output = (base / "output").lexically_normal();

    BLITZAR_CHECK(WriteConfig(config, OutputConfig));

    const CapturedOutput first = RunCaptured(config);
    const std::string expected = ExpectedSummary(output.generic_string(), 3U);

    BLITZAR_CHECK(first.exit_code == 0);
    BLITZAR_CHECK(first.standard_error.empty());
    BLITZAR_CHECK(first.standard_output == expected);
    BLITZAR_CHECK(std::filesystem::is_regular_file(output / "manifest.json"));
    BLITZAR_CHECK(std::filesystem::is_directory(output / "states"));

    const CapturedOutput rerun = RunCaptured(config);

    BLITZAR_CHECK(rerun.exit_code == static_cast<int>(blitzar_cli::BlitzarExitCode::Output));
    BLITZAR_CHECK(rerun.standard_output.empty());
    BLITZAR_CHECK(rerun.standard_error ==
                  "{\"schema_version\":1,\"status\":1,\"phase\":\"output-prepare\","
                  "\"exit_code\":5,\"message\":\"invalid argument\"}\n");

    return 0;
}

int CheckConfigurationFailure(const std::filesystem::path& base)
{
    RemoveTree(base);
    std::filesystem::create_directories(base);

    const std::filesystem::path config = base / "invalid.ini";

    BLITZAR_CHECK(WriteConfig(config, "unsupported_directive()\n"));

    const CapturedOutput result = RunCaptured(config);

    BLITZAR_CHECK(
        result.exit_code == static_cast<int>(blitzar_cli::BlitzarExitCode::Configuration));

    BLITZAR_CHECK(result.standard_output.empty());
    BLITZAR_CHECK(result.standard_error ==
                  "{\"schema_version\":1,\"status\":5,\"phase\":\"semantic\","
                  "\"exit_code\":3,\"message\":\"unsupported\"}\n");

    return 0;
}

} // namespace

int main()
{
    const std::filesystem::path base =
        std::filesystem::temp_directory_path() / "blitzar-cli-summary-646";

    BLITZAR_CHECK(CheckNoOutputSummary(base / "no-output") == 0);
    BLITZAR_CHECK(CheckOutputSummary(base / "output") == 0);
    BLITZAR_CHECK(CheckConfigurationFailure(base / "failure") == 0);

    RemoveTree(base);

    return 0;
}
