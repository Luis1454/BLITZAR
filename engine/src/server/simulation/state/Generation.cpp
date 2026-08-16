/*
 * @file engine/src/server/simulation/state/Generation.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Dispatch initial-state generation to focused model strategies.
 */

#include "GenerationContext.hpp"

#include <string>

bool buildGeneratedStateSingle(std::vector<Particle>& particles, std::uint32_t particleCount,
                               const InitialStateConfig& config)
{
    GenerationContext context(particles, particleCount, config);
    const std::string mode = toLower(config.mode);
    if (buildPrimitiveModel(context, mode)) {
        return true;
    }
    if (mode == "plummer_sphere") {
        return buildPlummerModel(context);
    }
    if (mode == "cosmology") {
        return buildCosmologyModel(context);
    }
    if (mode == "galaxy" || mode == "galaxy_collision") {
        return buildGalaxyModel(context, mode);
    }
    return buildDiskModel(context);
}
