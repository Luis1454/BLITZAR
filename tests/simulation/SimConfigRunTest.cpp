#include "simulation/input/SimConfigRun.hpp"

#include "fixtures/FixtureCheck.hpp"
#include "simulation/input/SimConfigFile.hpp"
#include "simulation/input/SimConfigState.hpp"

#include <filesystem>
#include <string>
#include <string_view>

namespace {

constexpr std::string_view ValidSource =
    R"(simulation(particle_count=4, dt=0.01, solver=direct, integrator=leapfrog_kdk)
gravity(gravitational_constant=1.0, softening=0.01)
units(length_scale=1.0, mass_scale=1.0, time_scale=1.0)
generation(seed=42, deterministic=true)
run(steps=2)
)";

constexpr std::string_view DefaultRunSource =
    R"(simulation(particle_count=2, dt=0.1, solver=direct, integrator=leapfrog_kdk)
gravity(gravitational_constant=1.0, softening=0.0)
units(length_scale=1.0, mass_scale=1.0, time_scale=1.0)
generation(seed=1, deterministic=true)
)";

constexpr std::string_view BarnesHutFirstSource =
    R"(barnes_hut(opening_angle=0.7, max_particles=4, max_cells=64, leaf_capacity=4, max_depth=16)
simulation(particle_count=4, dt=0.01, solver=barnes_hut, integrator=leapfrog_kdk)
gravity(gravitational_constant=1.0, softening=0.01)
units(length_scale=1.0, mass_scale=1.0, time_scale=1.0)
generation(seed=42, deterministic=true)
)";

[[nodiscard]] blitzar_status ParseWithExtra(
    std::string_view extra, blitzar_sim::SimConfigFile& destination)
{
    std::string source{ValidSource};

    source.append(extra);

    return blitzar_sim::ParseConfig(source, destination);
}

int CheckValidConfiguration()
{
    blitzar_sim::SimConfigFile source;
    blitzar_sim::SimConfigRun config;

    BLITZAR_CHECK(blitzar_sim::ParseConfig(ValidSource, source) == BLITZAR_STATUS_OK);
    BLITZAR_CHECK(blitzar_sim::BuildRunConfig(source, config) == BLITZAR_STATUS_OK);
    BLITZAR_CHECK(config.particle_count == 4);
    BLITZAR_CHECK(config.steps == 2);
    BLITZAR_CHECK(config.seed == 42U);
    BLITZAR_CHECK(config.solver == BLITZAR_SOLVER_DIRECT);
    BLITZAR_CHECK(config.integrator == BLITZAR_INTEGRATOR_LEAPFROG_KDK);
    BLITZAR_CHECK(config.barnes_hut.max_particles == 4);
    BLITZAR_CHECK(config.barnes_hut.max_cells == 33);

    blitzar_sim::SimConfigState state;

    BLITZAR_CHECK(blitzar_sim::BuildState(config, state) == BLITZAR_STATUS_OK);

    const blitzar_core::ParticleStateView input = state.Input();

    BLITZAR_CHECK(blitzar_core::IsValid(input));
    BLITZAR_CHECK(input.x.size() == 4U);
    BLITZAR_CHECK(input.mass[0] == 1.0);
    BLITZAR_CHECK(input.x[0] != input.x[1]);

    BLITZAR_CHECK(blitzar_sim::ParseConfig(DefaultRunSource, source) == BLITZAR_STATUS_OK);
    BLITZAR_CHECK(blitzar_sim::BuildRunConfig(source, config) == BLITZAR_STATUS_OK);
    BLITZAR_CHECK(config.steps == 1);
    BLITZAR_CHECK(!config.output.enabled);
    BLITZAR_CHECK(!config.diagnostics.enabled);

    BLITZAR_CHECK(blitzar_sim::ParseConfig(BarnesHutFirstSource, source) == BLITZAR_STATUS_OK);
    BLITZAR_CHECK(blitzar_sim::BuildRunConfig(source, config) == BLITZAR_STATUS_OK);
    BLITZAR_CHECK(config.solver == BLITZAR_SOLVER_BARNES_HUT);
    BLITZAR_CHECK(config.barnes_hut.opening_angle == 0.7);
    BLITZAR_CHECK(config.barnes_hut.max_cells == 64);

    return 0;
}

int CheckRejectedConfiguration()
{
    constexpr std::string_view unsupported =
        R"(simulation(particle_count=2, dt=0.1, solver=pm, integrator=leapfrog_kdk)
gravity(gravitational_constant=1.0, softening=0.0)
units(length_scale=1.0, mass_scale=1.0, time_scale=1.0)
generation(seed=1, deterministic=true)
)";

    constexpr std::string_view unknown_directive =
        R"(simulation(particle_count=2, dt=0.1, solver=direct, integrator=leapfrog_kdk)
gravity(gravitational_constant=1.0, softening=0.0)
units(length_scale=1.0, mass_scale=1.0, time_scale=1.0)
generation(seed=1, deterministic=true)
object(id=first)
)";

    constexpr std::string_view duplicate =
        R"(simulation(particle_count=2, dt=0.1, solver=direct, integrator=leapfrog_kdk)
simulation(particle_count=2, dt=0.1, solver=direct, integrator=leapfrog_kdk)
gravity(gravitational_constant=1.0, softening=0.0)
units(length_scale=1.0, mass_scale=1.0, time_scale=1.0)
generation(seed=1, deterministic=true)
)";

    constexpr std::string_view invalid_value =
        R"(simulation(particle_count=2, dt=0.1, solver=direct, integrator=leapfrog_kdk)
gravity(gravitational_constant=1.0, softening=-1.0)
units(length_scale=1.0, mass_scale=1.0, time_scale=1.0)
generation(seed=1, deterministic=true)
)";

    blitzar_sim::SimConfigRun config;

    config.steps = 73;

    blitzar_sim::SimConfigFile source;

    BLITZAR_CHECK(blitzar_sim::ParseConfig(unsupported, source) == BLITZAR_STATUS_OK);
    BLITZAR_CHECK(blitzar_sim::BuildRunConfig(source, config) == BLITZAR_STATUS_UNSUPPORTED);
    BLITZAR_CHECK(config.steps == 73);
    BLITZAR_CHECK(blitzar_sim::ParseConfig(unknown_directive, source) == BLITZAR_STATUS_OK);
    BLITZAR_CHECK(blitzar_sim::BuildRunConfig(source, config) == BLITZAR_STATUS_UNSUPPORTED);
    BLITZAR_CHECK(blitzar_sim::ParseConfig(duplicate, source) == BLITZAR_STATUS_OK);
    BLITZAR_CHECK(blitzar_sim::BuildRunConfig(source, config) == BLITZAR_STATUS_INVALID_ARGUMENT);
    BLITZAR_CHECK(blitzar_sim::ParseConfig(invalid_value, source) == BLITZAR_STATUS_OK);
    BLITZAR_CHECK(blitzar_sim::BuildRunConfig(source, config) == BLITZAR_STATUS_INVALID_ARGUMENT);
    BLITZAR_CHECK(config.steps == 73);

    return 0;
}

int CheckOutputConfiguration()
{
    constexpr std::string_view directives =
        R"(output(directory="results", every_steps=2, write_initial=true, write_final=true)
diagnostics(every_steps=3, energy=true, momentum=true, relative_error=false)
)";

    blitzar_sim::SimConfigFile source;
    blitzar_sim::SimConfigRun config;

    BLITZAR_CHECK(ParseWithExtra(directives, source) == BLITZAR_STATUS_OK);
    BLITZAR_CHECK(blitzar_sim::BuildRunConfig(source, std::filesystem::path{"case"}, config) ==
                  BLITZAR_STATUS_OK);

    BLITZAR_CHECK(config.output.enabled);
    BLITZAR_CHECK(config.output.directory == std::filesystem::path{"case/results"});
    BLITZAR_CHECK(config.output.every_steps == 2);
    BLITZAR_CHECK(config.output.write_initial);
    BLITZAR_CHECK(config.output.write_final);
    BLITZAR_CHECK(config.diagnostics.enabled);
    BLITZAR_CHECK(config.diagnostics.every_steps == 3);
    BLITZAR_CHECK(config.diagnostics.energy);
    BLITZAR_CHECK(config.diagnostics.momentum);
    BLITZAR_CHECK(!config.diagnostics.relative_error);

    return 0;
}

int CheckRejectedOutput(std::string_view directives)
{
    blitzar_sim::SimConfigFile source;
    blitzar_sim::SimConfigRun config;

    config.steps = 73;
    config.output.enabled = true;
    config.output.directory = std::filesystem::path{"sentinel"};

    BLITZAR_CHECK(ParseWithExtra(directives, source) == BLITZAR_STATUS_OK);
    BLITZAR_CHECK(blitzar_sim::BuildRunConfig(source, std::filesystem::path{"case"}, config) ==
                  BLITZAR_STATUS_INVALID_ARGUMENT);

    BLITZAR_CHECK(config.steps == 73);
    BLITZAR_CHECK(config.output.enabled);
    BLITZAR_CHECK(config.output.directory == std::filesystem::path{"sentinel"});

    return 0;
}

int CheckRejectedOutputConfiguration()
{
    BLITZAR_CHECK(
        CheckRejectedOutput(
            R"(output(directory=results, every_steps=1, write_initial=true, write_final=true)
)") == 0);

    BLITZAR_CHECK(CheckRejectedOutput(
                      R"(output(directory="", every_steps=1, write_initial=true, write_final=true)
)") == 0);

    BLITZAR_CHECK(
        CheckRejectedOutput(
            R"(output(directory="results", every_steps=0, write_initial=true, write_final=true)
)") == 0);

    BLITZAR_CHECK(
        CheckRejectedOutput(
            R"(output(directory="results", every_steps=1, write_initial=false, write_final=false)
)") == 0);

    BLITZAR_CHECK(
        CheckRejectedOutput(
            R"(diagnostics(every_steps=1, energy=false, momentum=false, relative_error=false)
)") == 0);

    BLITZAR_CHECK(
        CheckRejectedOutput(
            R"(output(directory="results", every_steps=1, write_initial=true, write_final=true, format=hdf5)
)") == 0);

    BLITZAR_CHECK(
        CheckRejectedOutput(
            R"(output(directory="results", every_steps=1, write_initial=true, write_final=true)
output(directory="other", every_steps=1, write_initial=true, write_final=true)
)") == 0);

    return 0;
}

} // namespace

int main()
{
    BLITZAR_CHECK(CheckValidConfiguration() == 0);
    BLITZAR_CHECK(CheckRejectedConfiguration() == 0);
    BLITZAR_CHECK(CheckOutputConfiguration() == 0);
    BLITZAR_CHECK(CheckRejectedOutputConfiguration() == 0);

    return 0;
}
