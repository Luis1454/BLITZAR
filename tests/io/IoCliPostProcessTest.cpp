#include "BlitzarPostProcess.hpp"
#include "BlitzarRun.hpp"
#include "BlitzarSummary.hpp"
#include "fixtures/FixtureCheck.hpp"
#include "fixtures/FixtureProcess.hpp"
#include "fixtures/FixtureRestart.hpp"

#include <blitzar/blitzar.hpp>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace {

constexpr std::string_view Config =
    R"(simulation(particle_count=4, dt=0.01, solver=direct, integrator=leapfrog_kdk)
gravity(gravitational_constant=1.0, softening=0.01)
units(length_scale=1.0, mass_scale=1.0, time_scale=1.0)
generation(seed=42, deterministic=true)
run(steps=4)
output(directory="output", every_steps=1, write_initial=true, write_final=true)
diagnostics(every_steps=1, energy=true, momentum=true, relative_error=true)
)";

struct CapturedOutput final {
    int exit_code{};
    std::string standard_output;
    std::string standard_error;
};

bool WriteConfig(const std::filesystem::path& path)
{
    std::ofstream output(path, std::ios::binary | std::ios::trunc);

    if (!output.is_open()) {
        return false;
    }

    output << Config;

    return static_cast<bool>(output);
}

bool WriteText(const std::filesystem::path& path, std::string_view text)
{
    std::ofstream output(path, std::ios::binary | std::ios::trunc);

    if (!output.is_open()) {
        return false;
    }

    output << text;

    return static_cast<bool>(output);
}

std::string ReadText(const std::filesystem::path& path)
{
    std::ifstream input(path, std::ios::binary);

    if (!input.is_open()) {
        return {};
    }

    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
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

CapturedOutput RunConfigCaptured(
    const std::filesystem::path& path, blitzar_cli::BlitzarOutputFormat format)
{
    std::ostringstream standard_output;
    std::ostringstream standard_error;
    const int exit_code = blitzar_cli::RunConfig(path, {standard_output, standard_error, format});

    return {exit_code, standard_output.str(), standard_error.str()};
}

CapturedOutput RunPostProcessCaptured(
    const std::filesystem::path& path, blitzar_cli::BlitzarOutputFormat format)
{
    std::ostringstream standard_output;
    std::ostringstream standard_error;
    const int exit_code =
        blitzar_cli::RunPostProcess(path, {standard_output, standard_error, format});

    return {exit_code, standard_output.str(), standard_error.str()};
}

int PrepareRun(const std::filesystem::path& directory)
{
    BLITZAR_CHECK(blitzar_test::EnsureDirectory(directory));
    BLITZAR_CHECK(WriteConfig(directory / "run.ini"));

    const CapturedOutput result =
        RunConfigCaptured(directory / "run.ini", blitzar_cli::BlitzarOutputFormat::Json);

    BLITZAR_CHECK(result.exit_code == 0);
    BLITZAR_CHECK(result.standard_error.empty());
    BLITZAR_CHECK(std::filesystem::is_regular_file(
        directory / "output" / "diagnostics" / "conservation.csv"));

    return 0;
}

int CheckParity(const std::filesystem::path& directory, const std::filesystem::path& executable)
{
    BLITZAR_CHECK(PrepareRun(directory) == 0);

    const std::filesystem::path output = directory / "output";
    const std::filesystem::path online_path = output / "diagnostics" / "conservation.csv";
    const std::filesystem::path post_path = output / "postProcessing" / "conservation.csv";

    const std::string online = ReadText(online_path);

    BLITZAR_CHECK(!online.empty());

    const CapturedOutput human =
        RunPostProcessCaptured(output, blitzar_cli::BlitzarOutputFormat::Human);

    const std::string expected_human = "BLITZAR result\n"
                                       "  status:       ok\n"
                                       "  solver:       direct\n"
                                       "  steps:        4/4\n"
                                       "  particles:    4\n"
                                       "  snapshots:    5\n"
                                       "  diagnostics:  5\n"
                                       "  output:       " +
                                       post_path.lexically_normal().generic_string() + "\n";

    BLITZAR_CHECK(human.exit_code == 0);
    BLITZAR_CHECK(human.standard_error.empty());
    BLITZAR_CHECK(human.standard_output == expected_human);

    BLITZAR_CHECK(blitzar_test::RemoveTree(output / "postProcessing"));

    const CapturedOutput direct =
        RunPostProcessCaptured(output, blitzar_cli::BlitzarOutputFormat::Json);

    BLITZAR_CHECK(direct.exit_code == 0);
    BLITZAR_CHECK(direct.standard_error.empty());
    BLITZAR_CHECK(ReadText(post_path) == online);

    BLITZAR_CHECK(blitzar_test::RemoveTree(output / "postProcessing"));
    BLITZAR_CHECK(blitzar_test::RunProcess(executable, "--post-process", output));
    BLITZAR_CHECK(ReadText(post_path) == online);

    const CapturedOutput rerun =
        RunPostProcessCaptured(output, blitzar_cli::BlitzarOutputFormat::Json);

    BLITZAR_CHECK(rerun.exit_code == static_cast<int>(blitzar_cli::BlitzarExitCode::Output));
    BLITZAR_CHECK(rerun.standard_output.empty());

    return 0;
}

int CheckFailure(const std::filesystem::path& directory, bool expect_absent = true)
{
    const CapturedOutput result =
        RunPostProcessCaptured(directory, blitzar_cli::BlitzarOutputFormat::Json);

    BLITZAR_CHECK(result.exit_code == static_cast<int>(blitzar_cli::BlitzarExitCode::Output));
    BLITZAR_CHECK(result.standard_output.empty());
    BLITZAR_CHECK(!result.standard_error.empty());
    BLITZAR_CHECK(expect_absent == !std::filesystem::exists(directory / "postProcessing"));

    return 0;
}

int CheckRejections(const std::filesystem::path& base)
{
    const std::filesystem::path missing = base / "missing";

    BLITZAR_CHECK(blitzar_test::EnsureDirectory(missing));
    BLITZAR_CHECK(CheckFailure(missing) == 0);

    const std::filesystem::path corrupt = base / "corrupt";

    BLITZAR_CHECK(PrepareRun(corrupt) == 0);

    const std::filesystem::path corrupt_state =
        corrupt / "output" / "states" / "state-00000001.bin";

    std::vector<std::uint8_t> bytes = ReadBytes(corrupt_state);

    BLITZAR_CHECK(!bytes.empty());

    bytes.back() ^= 1U;

    BLITZAR_CHECK(WriteBytes(corrupt_state, bytes));
    BLITZAR_CHECK(CheckFailure(corrupt / "output") == 0);

    const std::filesystem::path mixed = base / "mixed";

    BLITZAR_CHECK(PrepareRun(mixed) == 0);

    const std::filesystem::path manifest = mixed / "output" / "manifest.json";
    std::string manifest_text = ReadText(manifest);
    const std::string product = std::string(blitzar::version());
    const std::string product_field = "\"product_version\": \"" + product + "\"";

    BLITZAR_CHECK(manifest_text.find(product_field) != std::string::npos);

    manifest_text.replace(
        manifest_text.find(product_field), product_field.size(), "\"product_version\": \"9.9.9\"");

    BLITZAR_CHECK(WriteText(manifest, manifest_text));
    BLITZAR_CHECK(CheckFailure(mixed / "output") == 0);

    const std::filesystem::path extra = base / "extra";

    BLITZAR_CHECK(PrepareRun(extra) == 0);
    BLITZAR_CHECK(std::filesystem::copy_file(extra / "output" / "states" / "state-00000001.bin",
        extra / "output" / "states" / "state-99999999.bin"));

    BLITZAR_CHECK(CheckFailure(extra / "output") == 0);

    const std::filesystem::path ordered = base / "ordered";

    BLITZAR_CHECK(PrepareRun(ordered) == 0);

    const std::filesystem::path ordered_manifest = ordered / "output" / "manifest.json";
    std::string ordered_text = ReadText(ordered_manifest);
    const std::string first =
        "    {\n      \"step\": 1,\n      \"path\": \"states/state-00000001.bin\"\n    },";

    const std::string second =
        "    {\n      \"step\": 2,\n      \"path\": \"states/state-00000002.bin\"\n    },";

    const std::size_t first_position = ordered_text.find(first);

    BLITZAR_CHECK(first_position != std::string::npos);

    ordered_text.replace(first_position, first.size(), second);

    const std::size_t second_position = ordered_text.find(second, first_position + second.size());

    BLITZAR_CHECK(second_position != std::string::npos);

    ordered_text.replace(second_position, second.size(), first);

    BLITZAR_CHECK(WriteText(ordered_manifest, ordered_text));
    BLITZAR_CHECK(CheckFailure(ordered / "output") == 0);

    const std::filesystem::path occupied = base / "occupied";

    BLITZAR_CHECK(PrepareRun(occupied) == 0);
    BLITZAR_CHECK(blitzar_test::EnsureDirectory(occupied / "output" / "postProcessing"));
    BLITZAR_CHECK(WriteConfig(occupied / "output" / "postProcessing" / "existing.txt"));
    BLITZAR_CHECK(CheckFailure(occupied / "output", false) == 0);

    return 0;
}

} // namespace

int main(int argc, char** argv)
{
    if (argc != 2) {
        return 1;
    }

    std::filesystem::path base;

    if (!blitzar_test::AcquireTestDirectory(base, "blitzar-cli-post-process-648-")) {
        return 1;
    }

    const int parity_result = CheckParity(base / "parity", argv[1]);
    const int rejection_result = parity_result == 0 ? CheckRejections(base / "rejections") : 1;
    const bool cleanup_succeeded = blitzar_test::RemoveTree(base);

    return parity_result == 0 && rejection_result == 0 && cleanup_succeeded ? 0 : 1;
}
