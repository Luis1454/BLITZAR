/*
 * @file engine/config/directive/CfgLegacyScene.cpp
 * @brief Compatibility handling for legacy scene modifiers and properties.
 */

#include "CfgDirectiveInternals.hpp"

#include "config/core/CfgConfig.hpp"

#include <algorithm>

namespace bltzr_config {
struct LegacySceneProperty {
    std::string type = "transform";
    bool enabled = true;
    float offsetX = 0.0f;
    float offsetY = 0.0f;
    float offsetZ = 0.0f;
    float rotationX = 0.0f;
    float rotationY = 0.0f;
    float rotationZ = 0.0f;
    std::string axis = "z";
    std::uint32_t copies = 1u;
    bool mirrorX = false;
    bool mirrorY = false;
    bool mirrorZ = false;
    std::string pivot = "world";
    float pivotX = 0.0f;
    float pivotY = 0.0f;
    float pivotZ = 0.0f;
    std::string distribution = "uniform_sphere";
    std::uint32_t particleCount = 0u;
    std::uint32_t seed = 42u;
    float particleSize = 1.0f;
    float particleHeight = 1.0f;
    float particleMass = 0.001f;
    float particleSpeed = 0.0f;
    std::string emitterObjectId;
    std::string instanceObjectId;
};

static bool applyLegacyPropertyArg(const DirectiveArgument& arg, LegacySceneProperty& property)
{
    const std::string& key = arg.first;
    const std::string& value = arg.second;
    if (key == "type")
        property.type = value;
    else if (key == "enabled")
        return parseSceneBool(value, property.enabled);
    else if (key == "offset_x")
        return parseSceneFloat(value, property.offsetX);
    else if (key == "offset_y")
        return parseSceneFloat(value, property.offsetY);
    else if (key == "offset_z")
        return parseSceneFloat(value, property.offsetZ);
    else if (key == "rotation_x")
        return parseSceneFloat(value, property.rotationX);
    else if (key == "rotation_y")
        return parseSceneFloat(value, property.rotationY);
    else if (key == "rotation_z")
        return parseSceneFloat(value, property.rotationZ);
    else if (key == "axis" || key == "copy_axis")
        property.axis = value;
    else if (key == "copies" || key == "rotation_copies")
        return parseSceneUint(value, property.copies);
    else if (key == "mirror_x")
        return parseSceneBool(value, property.mirrorX);
    else if (key == "mirror_y")
        return parseSceneBool(value, property.mirrorY);
    else if (key == "mirror_z")
        return parseSceneBool(value, property.mirrorZ);
    else if (key == "pivot")
        property.pivot = value;
    else if (key == "pivot_x")
        return parseSceneFloat(value, property.pivotX);
    else if (key == "pivot_y")
        return parseSceneFloat(value, property.pivotY);
    else if (key == "pivot_z")
        return parseSceneFloat(value, property.pivotZ);
    else if (key == "distribution")
        property.distribution = value;
    else if (key == "particle_count")
        return parseSceneUint(value, property.particleCount);
    else if (key == "seed")
        return parseSceneUint(value, property.seed);
    else if (key == "particle_size" || key == "size")
        return parseSceneFloat(value, property.particleSize);
    else if (key == "particle_height" || key == "height")
        return parseSceneFloat(value, property.particleHeight);
    else if (key == "particle_mass")
        return parseSceneFloat(value, property.particleMass);
    else if (key == "particle_speed")
        return parseSceneFloat(value, property.particleSpeed);
    else if (key == "emitter_object_id")
        property.emitterObjectId = value;
    else if (key == "instance_object_id")
        property.instanceObjectId = value;
    else
        return false;
    return true;
}

void applyLegacySceneProperty(const DirectiveArguments& args, SceneConfig& scene,
                              std::ostream& warnings)
{
    if (scene.objects.empty()) {
        warnings << "[config] property ignored because no scene object exists\n";
        return;
    }
    LegacySceneProperty property;
    for (const auto& arg : args) {
        if (!applyLegacyPropertyArg(arg, property)) {
            warnings << "[config] unknown or invalid property argument: " << arg.first << "\n";
        }
    }
    SceneObjectConfig& object = scene.objects.back();
    if (property.type == "particle_system") {
        SceneObjectConfig system;
        std::uint32_t sequence = 1u;
        bool hasMatchingId = true;
        while (hasMatchingId) {
            system.id = object.id + "_system_" + std::to_string(sequence++);
            hasMatchingId = std::any_of(scene.objects.begin(), scene.objects.end(),
                                        [&system](const SceneObjectConfig& candidate) {
                                            return candidate.id == system.id;
                                        });
        }
        system.name = object.name + " Particle System";
        system.type = "particle_system";
        system.enabled = property.enabled;
        system.particleCount = property.particleCount;
        system.seed = property.seed;
        system.particleMass = property.particleMass;
        system.particleSize = property.particleSize;
        system.particleHeight = property.particleHeight;
        system.particleSpeed = property.particleSpeed;
        system.distribution = property.distribution;
        system.emitterObjectId =
            property.emitterObjectId.empty() ? object.id : property.emitterObjectId;
        system.targetAssetId = property.instanceObjectId;
        scene.objects.push_back(std::move(system));
        return;
    }
    object.offsetX += property.offsetX;
    object.offsetY += property.offsetY;
    object.offsetZ += property.offsetZ;
    object.rotationX += property.rotationX;
    object.rotationY += property.rotationY;
    object.rotationZ += property.rotationZ;
    object.axis = property.axis;
    object.copies =
        std::min<std::uint32_t>(256u, object.copies * std::max<std::uint32_t>(1u, property.copies));
    object.mirrorX = object.mirrorX || property.mirrorX;
    object.mirrorY = object.mirrorY || property.mirrorY;
    object.mirrorZ = object.mirrorZ || property.mirrorZ;
    if (property.pivot != "world") {
        object.pivot = property.pivot;
        object.pivotX = property.pivotX;
        object.pivotY = property.pivotY;
        object.pivotZ = property.pivotZ;
    }
}
} // namespace bltzr_config
