#include "config/SimulationArgsCoreOptions.hpp"
#include "config/SimulationModes.hpp"
#include "config/SimulationOptionRegistry.hpp"

bool SimulationArgsCoreOptions::apply(
    const std::string &key,
    const std::string &value,
    SimulationConfig &config,
    RuntimeArgs &runtime,
    std::ostream &warnings
)
{
    if (key == "--config") {
        runtime.configPath = value;
        return true;
    }
    if (key == "--solver") {
        std::string canonical;
        if (!grav_modes::normalizeSolver(value, canonical)) {
            runtime.hasArgumentError = true;
            warnings << "[args] invalid --solver: " << value << " (allowed: pairwise_cuda|octree_gpu|octree_cpu)\n";
            return true;
        }
    }
    if (key == "--integrator") {
        std::string canonical;
        if (!grav_modes::normalizeIntegrator(value, canonical)) {
            runtime.hasArgumentError = true;
            warnings << "[args] invalid --integrator: " << value << " (allowed: euler|rk4)\n";
            return true;
        }
    }
    return grav_config::applyCliOption(grav_config::SimulationOptionGroup::Core, key, value, config, warnings);
}
