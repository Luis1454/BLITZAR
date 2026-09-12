#include "core/CoreArithmetic.hpp"
#include "core/CoreExecution.hpp"
#include "core/CoreSnapshot.hpp"
#include "fixtures/FixtureCheck.hpp"
#include "io/md/manifest/MetadataManifest.hpp"
#include "io/md/reader/MetadataReader.hpp"
#include "io/md/run/MetadataRun.hpp"
#include "physics/reduction/ScalarReduction.hpp"
#include "sdk/cpp/CppSimulationAccess.hpp"
#include "simulation/cfg/SimConfigFile.hpp"
#include "simulation/cfg/run/SimConfigRun.hpp"

#include <blitzar/cpp/blitzar.hpp>
#include <cmath>
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
    info.plan_version = "1.0.57";
    info.configuration.simulation = {
        2, 100, 0.1, BLITZAR_SOLVER_DIRECT, BLITZAR_INTEGRATOR_LEAPFROG_KDK};

    info.configuration.gravity = {1.0, 0.01};
    info.configuration.units = {1.0, 1.0, 1.0};
    info.configuration.barnes_hut = {0.5, 2, 17, 8, 32};
    info.configuration.generation = {42, true};
    info.configuration.execution = {blitzar_core::ExecutionMode::Fast,
        blitzar_core::ExecutionSettings::Fast().cpu, blitzar_core::ExecutionSettings::Fast().hip,
        blitzar_core::ExecutionSettings::Fast().mpi, "runtime-selected", "float64",
        blitzar_io::CurrentCompilerIdentity(), "runtime-selected", "seeded-jitter-v1",
        "backend-defined;diagnostics-neumaier-v1", "stable-particle-id-v1", false};

    return info;
}

void RemoveTree(const std::filesystem::path& path)
{
    std::error_code error;

    std::filesystem::remove_all(path, error);
}

int CheckExecutionSettings()
{
    const blitzar_core::ExecutionSettings strict = blitzar_core::ExecutionSettings::Strict();
    const blitzar_core::ExecutionSettings fast = blitzar_core::ExecutionSettings::Fast();

    BLITZAR_CHECK(strict.IsValid());
    BLITZAR_CHECK(strict.IsBitwiseReproducible());
    BLITZAR_CHECK(strict.cpu.fma == blitzar_core::FmaPolicy::Disabled);
    BLITZAR_CHECK(strict.cpu.reduction == blitzar_core::ReductionPolicy::Ordered);
    BLITZAR_CHECK(fast.IsValid());
    BLITZAR_CHECK(!fast.IsBitwiseReproducible());
    BLITZAR_CHECK(fast.hip.fma == blitzar_core::FmaPolicy::Hardware);
    BLITZAR_CHECK(fast.mpi.reduction == blitzar_core::ReductionPolicy::BackendDefined);
    BLITZAR_CHECK(!strict.AllowsAccelerator());
    BLITZAR_CHECK(fast.AllowsAccelerator());

    volatile blitzar_core::Scalar product = 0.1 * 0.2;
    const blitzar_core::Scalar disabled = 0.3 + product;

    BLITZAR_CHECK(blitzar_core::MultiplyAdd(0.1, 0.2, 0.3, strict.cpu) == disabled);
    BLITZAR_CHECK(blitzar_core::MultiplyAdd(0.1, 0.2, 0.3, fast.cpu) == std::fma(0.1, 0.2, 0.3));

    const blitzar_core::BackendExecutionPolicy compensated_policy{
        blitzar_core::FmaPolicy::Disabled, blitzar_core::ReductionPolicy::Compensated};

    blitzar_physics::ScalarReduction ordered(strict.cpu);
    blitzar_physics::ScalarReduction compensated(compensated_policy);

    for (const blitzar_core::Scalar value : {1.0e16, 1.0, -1.0e16}) {
        ordered.Add(value);
        compensated.Add(value);
    }

    BLITZAR_CHECK(ordered.Value() == 0.0);
    BLITZAR_CHECK(compensated.Value() == 1.0);

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
    BLITZAR_CHECK(config.seed == 42U);
    BLITZAR_CHECK(config.execution.IsBitwiseReproducible());

    BLITZAR_CHECK(blitzar_sim::ParseConfig(FastConfig, source) == BLITZAR_STATUS_OK);
    BLITZAR_CHECK(blitzar_sim::BuildRunConfig(source, config) == BLITZAR_STATUS_OK);
    BLITZAR_CHECK(config.execution.mode == blitzar_core::ExecutionMode::Fast);
    BLITZAR_CHECK(config.seed == 42U);
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

    BLITZAR_CHECK(parsed.configuration.execution.backend == "runtime-selected");
    BLITZAR_CHECK(parsed.configuration.execution.precision == "float64");
    BLITZAR_CHECK(parsed.configuration.execution.compiler == blitzar_io::CurrentCompilerIdentity());
    BLITZAR_CHECK(parsed.configuration.execution.rng == "seeded-jitter-v1");
    BLITZAR_CHECK(
        parsed.configuration.execution.compensator == "backend-defined;diagnostics-neumaier-v1");

    BLITZAR_CHECK(parsed.configuration.execution.ordering == "stable-particle-id-v1");
    BLITZAR_CHECK(!parsed.configuration.execution.bitwise_reproducible);
    BLITZAR_CHECK(completed_steps.empty());

    blitzar_io::MetadataRunInfo invalid = MakeFastInfo();

    invalid.configuration.execution.backend = "cpu";

    BLITZAR_CHECK(invalid.configuration.execution.Validate() == BLITZAR_STATUS_INVALID_ARGUMENT);

    RemoveTree(root);

    return 0;
}

int CheckSnapshotBoundary()
{
    BLITZAR_CHECK(blitzar_core::IsSnapshotBoundaryReady(false));
    BLITZAR_CHECK(!blitzar_core::IsSnapshotBoundaryReady(true));

    blitzar::Context context;

    BLITZAR_CHECK(context.valid());

    blitzar::Simulation simulation(context, 2);

    BLITZAR_CHECK(simulation.valid());
    BLITZAR_CHECK(blitzar::CppSimulationAccess::SetExecutionSettings(
                      simulation, blitzar_core::ExecutionSettings::Strict()) == BLITZAR_STATUS_OK);

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
