/*
 * @file engine/config/validation/src/validation/ScenarioScene.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Responsibility-focused scenario validation rules.
 */

#include "ScenarioInternals.hpp"

#include <algorithm>

namespace bltzr_config {

void validateSceneObjects(const SimulationConfig& config, ScenarioValidationContext& context)
{
    if (!config.scene.objects.empty()) {
        const std::size_t enabledObjects = static_cast<std::size_t>(
            std::count_if(config.scene.objects.begin(), config.scene.objects.end(),
                          [](const SceneObjectConfig& object) {
                              return object.enabled;
                          }));
        if (enabledObjects == 0u) {
            context.add(ScenarioDiagnosticLevel::Error, "scene.objects",
                        "The scene must contain at least one enabled object.",
                        "Enable an object or add a new scene object.");
        }
        for (const SceneObjectConfig& object : config.scene.objects) {
            if (object.enabled && object.particleCount == 0u) {
                context.add(ScenarioDiagnosticLevel::Error, "scene.objects.particle_count",
                            "Enabled scene objects must request at least one particle.",
                            "Set the object particle count to at least 1.");
            }
        }
    }
}

} // namespace bltzr_config
