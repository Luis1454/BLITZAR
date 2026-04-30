/*
 * @file engine/src/config/SimulationArgsCoreOptions.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Configuration parsing, validation, and serialization implementation.
 */

#include "config/SimulationArgsCoreOptions.hpp"
#include "config/SimulationModes.hpp"
#include "config/SimulationOptionRegistry.hpp"

/*
 * @brief Documents the apply operation contract.
 * @param key Input value used by this contract.
 * @param value Input value used by this contract.
 * @param config Input value used by this contract.
 * @param runtime Input value used by this contract.
 * @param warnings Input value used by this contract.
 * @return bool SimulationArgsCoreOptions:: value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
bool SimulationArgsCoreOptions::apply(const std::string& key, const std::string& value,
                                      SimulationConfig& config, RuntimeArgs& runtime,
                                      std::ostream& warnings)
{
    if (key == "--config") {
        runtime.configPath = value;
        return true;
    }
    if (key == "--solver") {
        std::string canonical;
        if (!bltzr_modes::normalizeSolver(value, canonical)) {
            runtime.hasArgumentError = true;
            warnings << "[args] invalid --solver: " << value
                     << " (allowed: pairwise_cuda|octree_gpu|octree_cpu)\n";
            return true;
        }
    }
    if (key == "--integrator") {
        std::string canonical;
        if (!bltzr_modes::normalizeIntegrator(value, canonical)) {
            runtime.hasArgumentError = true;
            warnings << "[args] invalid --integrator: " << value
                     << " (allowed: euler|rk4|leapfrog)\n";
            return true;
        }
    }
    return bltzr_config::applyCliOption(bltzr_config::SimulationOptionGroup::Core, key, value,
                                        config, warnings);
}
