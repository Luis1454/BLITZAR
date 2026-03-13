#include "tests/support/numerical_validation_tool.hpp"

#include <array>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <ostream>
#include <string>
#include <vector>

#include "tests/support/physics_scenario.hpp"

namespace grav_test_numerics_tool {
struct ToolOptions {
    std::string preset;
    std::string solver;
};
struct PresetConfig {
    testsupport::ScenarioConfig scenario{};
    std::string dataset;
    std::uint32_t seed = 0u;
};

float centerOfMassDrift(const std::vector<RenderParticle> &initial, const std::vector<RenderParticle> &final)
{
    const std::array<float, 3> start = testsupport::centerOfMassAll(initial);
    const std::array<float, 3> end = testsupport::centerOfMassAll(final);
    const float dx = end[0] - start[0];
    const float dy = end[1] - start[1];
    const float dz = end[2] - start[2];
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

void writeVector(std::ostream &out, const std::string &name, const std::array<float, 3> &value)
{
    out << name << "=" << value[0] << "," << value[1] << "," << value[2] << "\n";
}
} // namespace grav_test_numerics_tool

namespace grav_test_numerics {

bool parseArgs(int argc, const char *const *argv, grav_test_numerics_tool::ToolOptions &out, std::string &error)
{
    for (int index = 1; index < argc; index += 1) {
        const std::string arg(argv[index]);
        if (arg == "--help") {
            error = "usage: gravityNumericalValidationTool --preset <name> --solver <solver>";
            return false;
        }
        if (index + 1 >= argc) {
            error = "missing value for argument: " + arg;
            return false;
        }
        const std::string value(argv[index + 1]);
        if (arg == "--preset") {
            out.preset = value;
        } else if (arg == "--solver") {
            out.solver = value;
        } else {
            error = "unknown argument: " + arg;
            return false;
        }
        index += 1;
    }
    if (out.preset.empty() || out.solver.empty()) {
        error = "both --preset and --solver are required";
        return false;
    }
    return true;
}

bool applyPreset(
    const grav_test_numerics_tool::ToolOptions &options,
    grav_test_numerics_tool::PresetConfig &out,
    std::string &error)
{
    testsupport::ScenarioConfig cfg;
    cfg.solver = options.solver;
    if (options.preset == "two_body_orbit_drift" || options.preset == "two_body_orbit_convergence_coarse"
        || options.preset == "two_body_orbit_convergence_fine") {
        if (!testsupport::prepareTwoBodyScenario(cfg, error)) {
            return false;
        }
        cfg.dt = options.preset == "two_body_orbit_convergence_fine" ? 0.001f : 0.002f;
        cfg.steps = options.preset == "two_body_orbit_drift" ? 800u : (options.preset == "two_body_orbit_convergence_fine" ? 200u : 100u);
        out.dataset = "tests/data/two_body_rest.xyz";
    } else if (options.preset == "disk_solver_parity") {
        cfg.particleCount = 384u;
        cfg.dt = 0.004f;
        cfg.steps = 180u;
        cfg.integrator = "euler";
        cfg.energyMeasureEverySteps = 1u;
        cfg.energySampleLimit = 384u;
        cfg.snapshotTimeoutMs = 10000;
        cfg.stepTimeoutMs = 10000;
        cfg.octreeTheta = 0.35f;
        cfg.octreeSoftening = 0.08f;
        cfg.initState.mode = "disk_orbit";
        cfg.initState.seed = 12345u;
        cfg.initState.includeCentralBody = true;
        cfg.initState.centralMass = 1.0f;
        cfg.initState.diskMass = 0.75f;
        cfg.initState.diskRadiusMin = 1.5f;
        cfg.initState.diskRadiusMax = 11.5f;
        cfg.initState.diskThickness = 0.0f;
        cfg.initState.velocityScale = 1.0f;
        cfg.initState.velocityTemperature = 0.1f;
        cfg.initState.particleTemperature = 0.0f;
        cfg.initState.thermalAmbientTemperature = 0.0f;
        cfg.initState.thermalSpecificHeat = 1.0f;
        cfg.initState.thermalHeatingCoeff = 0.0f;
        cfg.initState.thermalRadiationCoeff = 0.0f;
        out.dataset = "generated:disk_orbit";
        out.seed = 12345u;
    } else if (options.preset == "radiation_exchange") {
        cfg.particleCount = 128u;
        cfg.dt = 0.1f;
        cfg.steps = 80u;
        cfg.integrator = "euler";
        cfg.energyMeasureEverySteps = 1u;
        cfg.energySampleLimit = 128u;
        cfg.snapshotTimeoutMs = 10000;
        cfg.stepTimeoutMs = 10000;
        cfg.initState.mode = "random_cloud";
        cfg.initState.seed = 7u;
        cfg.initState.includeCentralBody = false;
        cfg.initState.cloudHalfExtent = 50.0f;
        cfg.initState.cloudSpeed = 0.0f;
        cfg.initState.particleMass = 1e-6f;
        cfg.initState.velocityTemperature = 0.0f;
        cfg.initState.particleTemperature = 1000.0f;
        cfg.initState.thermalAmbientTemperature = 0.0f;
        cfg.initState.thermalSpecificHeat = 1.0f;
        cfg.initState.thermalHeatingCoeff = 0.0f;
        cfg.initState.thermalRadiationCoeff = 1.0f;
        out.dataset = "generated:random_cloud";
        out.seed = 7u;
    } else if (options.preset == "calibration_two_body") {
        if (!testsupport::prepareGeneratedCalibrationScenario("two_body", cfg, error)) {
            return false;
        }
        out.dataset = "generated:two_body";
        out.seed = cfg.initState.seed;
    } else if (options.preset == "calibration_three_body") {
        if (!testsupport::prepareGeneratedCalibrationScenario("three_body", cfg, error)) {
            return false;
        }
        out.dataset = "generated:three_body";
        out.seed = cfg.initState.seed;
    } else if (options.preset == "calibration_plummer") {
        if (!testsupport::prepareGeneratedCalibrationScenario("plummer_sphere", cfg, error)) {
            return false;
        }
        out.dataset = "generated:plummer_sphere";
        out.seed = cfg.initState.seed;
    } else {
        error = "unknown preset: " + options.preset;
        return false;
    }
    out.scenario = cfg;
    return true;
}

int NumericalValidationTool::run(int argc, const char *const *argv, std::ostream &out, std::ostream &err) const
{
    grav_test_numerics_tool::ToolOptions options;
    std::string error;
    if (!parseArgs(argc, argv, options, error)) {
        err << error << "\n";
        return error.rfind("usage:", 0) == 0 ? 0 : 1;
    }

    grav_test_numerics_tool::PresetConfig preset;
    if (!applyPreset(options, preset, error)) {
        err << error << "\n";
        return 1;
    }

    testsupport::ScenarioResult result;
    if (!testsupport::runScenario(preset.scenario, result, error)) {
        err << error << "\n";
        return 1;
    }

    out << std::fixed << std::setprecision(8);
    out << "preset=" << options.preset << "\n";
    out << "solver=" << options.solver << "\n";
    out << "dataset=" << preset.dataset << "\n";
    out << "seed=" << preset.seed << "\n";
    out << "steps_completed=" << result.stats.steps << "\n";
    out << "particle_count=" << result.final.size() << "\n";
    out << "energy_estimated=" << (result.stats.energyEstimated ? 1 : 0) << "\n";
    out << "max_abs_energy_drift_pct=" << result.maxAbsEnergyDriftPct << "\n";
    out << "average_radius=" << testsupport::averageRadius(result.final) << "\n";
    out << "total_energy=" << result.stats.totalEnergy << "\n";
    out << "thermal_energy=" << result.stats.thermalEnergy << "\n";
    out << "radiated_energy=" << result.stats.radiatedEnergy << "\n";
    out << "center_of_mass_drift=" << grav_test_numerics_tool::centerOfMassDrift(result.initial, result.final) << "\n";
    grav_test_numerics_tool::writeVector(out, "initial_center_of_mass", testsupport::centerOfMassAll(result.initial));
    grav_test_numerics_tool::writeVector(out, "final_center_of_mass", testsupport::centerOfMassAll(result.final));
    if (result.final.size() <= 8u) {
        for (std::size_t index = 0; index < result.final.size(); index += 1u) {
            const RenderParticle &particle = result.final[index];
            out << "final_particle_" << index << "=" << particle.x << "," << particle.y << "," << particle.z << "\n";
        }
    }
    return 0;
}

} // namespace grav_test_numerics
