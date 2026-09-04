#include "core/CoreExecution.hpp"
#include "fixtures/FixtureCheck.hpp"
#include "io/metadata/MetadataManifest.hpp"
#include "io/metadata/MetadataReader.hpp"
#include "io/metadata/MetadataRun.hpp"
#include "sdk/cpp/CppSimulationAccess.hpp"
#include "simulation/config/SimConfigFile.hpp"
#include "simulation/config/SimConfigRun.hpp"

#include <blitzar/blitzar.hpp>
#include <cstdint>
#include <filesystem>
#include <string_view>
#include <vector>

namespace {

constexpr std::string_view BaseConfig =
    R"(simulation(particle_count=2, dt=0.1, solver=direct, integrator=leapfrog_kdk)
gravity(gravitational_constant=1.0, softening=0.01)
units(length_scale=1.0, mass_scale=1.0, time_scale=1.0)
generation(seed=42, deterministic=true)
run(steps=100)
)";

constexpr std::string_view FastConfig =
    R"(simulation(particle_count=2, dt=0.1, solver=direct, integrator=leapfrog_kdk)
gravity(gravitational_constant=1.0, softening=0.01)
units(length_scale=1.0, mass_scale=1.0, time_scale=1.0)
generation(seed=42, deterministic=true)
execution(mode=fast)
run(steps=100)
)";

[[nodiscard]] blitzar_io::MetadataRunInfo MakeFastInfo()
{
    blitzar_io::MetadataRunInfo info;

    info.product_version = "1.0.0";
    info.plan_version = "1.0.55";
    info.configuration.simulation = {
        2, 100, 0.1, BLITZAR_SOLVER_DIRECT, BLITZAR_INTEGRATOR_LEAPFROG_KDK};

    info.configuration.gravity = {1.0, 0.01};
    info.configuration.units = {1.0, 1.0, 1.0};
    info.configuration.barnes_hut = {0.5, 2, 17, 8, 32};
    info.configuration.generation = {42, true};
    info.configuration.execution = {blitzar_core::ExecutionMode::Fast,
        blitzar_core::ExecutionSettings::Fast(42).cpu,
        blitzar_core::ExecutionSettings::Fast(42).hip,
        blitzar_core::ExecutionSettings::Fast(42).mpi, "float64",
        blitzar_io::CurrentCompilerIdentity(), "host-cpu", "seeded-jitter-v1",
        "direct-plain;diagnostics-neumaier-v1", "stable-particle-id-v1", false};

    return info;
}

void RemoveTree(const std::filesystem::path& path)
{
    std::error_code error;

    std::filesystem::remove_all(path, error);
}

int CheckExecutionSettings()
{
    const blitzar_core::ExecutionSettings strict = blitzar_core::ExecutionSettings::Strict(42);
    const blitzar_core::ExecutionSettings fast = blitzar_core::ExecutionSettings::Fast(42);

    BLITZAR_CHECK(strict.IsValid());
    BLITZAR_CHECK(strict.IsBitwiseReproducible());
    BLITZAR_CHECK(strict.cpu.fma == blitzar_core::FmaPolicy::Disabled);
    BLITZAR_CHECK(strict.cpu.reduction == blitzar_core::ReductionPolicy::Ordered);
    BLITZAR_CHECK(fast.IsValid());
    BLITZAR_CHECK(!fast.IsBitwiseReproducible());
    BLITZAR_CHECK(fast.hip.fma == blitzar_core::FmaPolicy::Hardware);
    BLITZAR_CHECK(fast.mpi.reduction == blitzar_core::ReductionPolicy::BackendDefined);

    blitzar_core::ExecutionSettings invalid = strict;

    invalid.cpu.fma = blitzar_core::FmaPolicy::Hardware;

    BLITZAR_CHECK(!invalid.IsValid());
    BLITZAR_CHECK(blitzar_core::ExecutionModeName(strict.mode) == "strict");
    BLITZAR_CHECK(blitzar_core::ExecutionModeName(fast.mode) == "fast");

    return 0;
}

int CheckConfiguration()
{
    blitzar_sim::SimConfigFile source;
    blitzar_sim::SimConfigRun config;

    BLITZAR_CHECK(blitzar_sim::ParseConfig(BaseConfig, source) == BLITZAR_STATUS_OK);
    BLITZAR_CHECK(blitzar_sim::BuildRunConfig(source, config) == BLITZAR_STATUS_OK);
    BLITZAR_CHECK(config.execution.mode == blitzar_core::ExecutionMode::Strict);
    BLITZAR_CHECK(config.execution.seed == 42U);
    BLITZAR_CHECK(config.execution.IsBitwiseReproducible());

    BLITZAR_CHECK(blitzar_sim::ParseConfig(FastConfig, source) == BLITZAR_STATUS_OK);
    BLITZAR_CHECK(blitzar_sim::BuildRunConfig(source, config) == BLITZAR_STATUS_OK);
    BLITZAR_CHECK(config.execution.mode == blitzar_core::ExecutionMode::Fast);
    BLITZAR_CHECK(config.execution.seed == 42U);
    BLITZAR_CHECK(!config.execution.IsBitwiseReproducible());

    constexpr std::string_view invalid_mode =
        R"(simulation(particle_count=2, dt=0.1, solver=direct, integrator=leapfrog_kdk)
gravity(gravitational_constant=1.0, softening=0.01)
units(length_scale=1.0, mass_scale=1.0, time_scale=1.0)
generation(seed=42, deterministic=true)
execution(mode=portable)
)";

    BLITZAR_CHECK(blitzar_sim::ParseConfig(invalid_mode, source) == BLITZAR_STATUS_OK);
    BLITZAR_CHECK(blitzar_sim::BuildRunConfig(source, config) == BLITZAR_STATUS_INVALID_ARGUMENT);

    return 0;
}

int CheckManifest(const std::filesystem::path& root)
{
    RemoveTree(root);

    blitzar_io::MetadataRun run(root, MakeFastInfo());

    BLITZAR_CHECK(run.Prepare() == BLITZAR_STATUS_OK);

    blitzar_io::MetadataReader reader;
    blitzar_io::MetadataRunInfo parsed;
    std::vector<std::uint64_t> completed_steps;

    BLITZAR_CHECK(reader.Read(run.ManifestPath(), parsed, completed_steps) == BLITZAR_STATUS_OK);
    BLITZAR_CHECK(parsed.configuration.execution.mode == blitzar_core::ExecutionMode::Fast);
    BLITZAR_CHECK(parsed.configuration.execution.cpu.fma == blitzar_core::FmaPolicy::Hardware);
    BLITZAR_CHECK(parsed.configuration.execution.mpi.reduction ==
                  blitzar_core::ReductionPolicy::BackendDefined);

    BLITZAR_CHECK(parsed.configuration.execution.precision == "float64");
    BLITZAR_CHECK(parsed.configuration.execution.compiler == blitzar_io::CurrentCompilerIdentity());
    BLITZAR_CHECK(parsed.configuration.execution.rng == "seeded-jitter-v1");
    BLITZAR_CHECK(
        parsed.configuration.execution.compensator == "direct-plain;diagnostics-neumaier-v1");

    BLITZAR_CHECK(parsed.configuration.execution.ordering == "stable-particle-id-v1");
    BLITZAR_CHECK(!parsed.configuration.execution.bitwise_reproducible);
    BLITZAR_CHECK(completed_steps.empty());

    RemoveTree(root);

    return 0;
}

int CheckSnapshotBoundary()
{
    blitzar::Context context;

    BLITZAR_CHECK(context.valid());

    blitzar::Simulation simulation(context, 2);

    BLITZAR_CHECK(simulation.valid());
    BLITZAR_CHECK(blitzar::CppSimulationAccess::SetExecutionSettings(simulation,
                      blitzar_core::ExecutionSettings::Strict(42)) == BLITZAR_STATUS_OK);

    BLITZAR_CHECK(blitzar::CppSimulationAccess::IsSnapshotBoundaryReady(simulation));

    return 0;
}

} // namespace

int main()
{
    const std::filesystem::path root =
        std::filesystem::temp_directory_path() / "blitzar-reproducibility-685";

    BLITZAR_CHECK(CheckExecutionSettings() == 0);
    BLITZAR_CHECK(CheckConfiguration() == 0);
    BLITZAR_CHECK(CheckManifest(root) == 0);
    BLITZAR_CHECK(CheckSnapshotBoundary() == 0);

    RemoveTree(root);

    return 0;
}
