#include "simulation/config/SimConfigDirective.hpp"
#include "simulation/config/SimConfigValue.hpp"

#include <array>
#include <cmath>
#include <string_view>

namespace blitzar_sim {

namespace {

[[nodiscard]] blitzar_status ParseSolver(
    std::string_view text, blitzar_solver_kind& solver) noexcept
{
    if (text == "direct") {
        solver = BLITZAR_SOLVER_DIRECT;

        return BLITZAR_STATUS_OK;
    }

    if (text == "barnes_hut") {
        solver = BLITZAR_SOLVER_BARNES_HUT;

        return BLITZAR_STATUS_OK;
    }

    if (text == "fmm") {
        solver = BLITZAR_SOLVER_FMM;

        return BLITZAR_STATUS_OK;
    }

    if (text == "pm") {
        solver = BLITZAR_SOLVER_PM;

        return BLITZAR_STATUS_OK;
    }

    if (text == "treepm") {
        solver = BLITZAR_SOLVER_TREEPM;

        return BLITZAR_STATUS_OK;
    }

    if (text == "kifmm") {
        solver = BLITZAR_SOLVER_KIFMM;

        return BLITZAR_STATUS_OK;
    }

    return BLITZAR_STATUS_INVALID_ARGUMENT;
}

[[nodiscard]] blitzar_status ParseIntegrator(
    std::string_view text, blitzar_integrator_kind& integrator) noexcept
{
    if (text == "leapfrog_kdk") {
        integrator = BLITZAR_INTEGRATOR_LEAPFROG_KDK;

        return BLITZAR_STATUS_OK;
    }

    if (text == "euler") {
        return BLITZAR_STATUS_UNSUPPORTED;
    }

    return BLITZAR_STATUS_INVALID_ARGUMENT;
}

} // namespace

blitzar_status ApplySimulationDirective(
    const SimConfigFile::Directive& directive, SimConfigRun& config) noexcept
{
    constexpr std::array<std::string_view, 4> names{"particle_count", "dt", "solver", "integrator"};
    std::int64_t particle_count = 0;
    double timestep = 0.0;
    std::string_view solver_text;
    std::string_view integrator_text;

    if (!HasExactArguments(directive, names) ||
        !ReadConfigInteger(directive, "particle_count", particle_count) ||
        !ReadConfigReal(directive, "dt", timestep) ||
        !ReadConfigText(directive, "solver", solver_text) ||
        !ReadConfigText(directive, "integrator", integrator_text)) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    if (particle_count <= 0 || particle_count > SimConfigRun::MaxParticleCount ||
        !std::isfinite(timestep) || timestep <= 0.0) {
        return BLITZAR_STATUS_INVALID_ARGUMENT;
    }

    blitzar_solver_kind solver{};
    blitzar_integrator_kind integrator{};
    const blitzar_status solver_status = ParseSolver(solver_text, solver);

    if (solver_status != BLITZAR_STATUS_OK) {
        return solver_status;
    }

    const blitzar_status integrator_status = ParseIntegrator(integrator_text, integrator);

    if (integrator_status != BLITZAR_STATUS_OK) {
        return integrator_status;
    }

    config.particle_count = particle_count;
    config.timestep = timestep;
    config.solver = solver;
    config.integrator = integrator;

    return BLITZAR_STATUS_OK;
}

} // namespace blitzar_sim
