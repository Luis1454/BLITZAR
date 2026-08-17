/*
 * @file engine/src/config/directive/SceneWriter.cpp
 * @brief Serialization of scene and scene transform directives.
 */

#include "WriteInternals.hpp"

#include "config/core/Config.hpp"
#include "config/directive/StreamWriter.hpp"

namespace bltzr_config {
void writeScene(std::ostream& out, const SimulationConfig& config)
{
    DirectiveStreamWriter writer(out, "scene");
    writer.writeString("style", config.initConfigStyle);
    writer.writeString("preset", config.presetStructure);
    writer.writeString("mode", config.initMode);
    writer.writeQuotedString("file", config.inputFile);
    writer.writeString("format", config.inputFormat);
    writer.finish();
}

void writeSceneObjects(std::ostream& out, const SimulationConfig& config)
{
    for (const SceneObjectConfig& object : config.scene.objects) {
        DirectiveStreamWriter writer(out, "object");
        writer.writeQuotedString("id", object.id);
        writer.writeQuotedString("name", object.name);
        writer.writeString("type", object.type);
        writer.writeBool("enabled", object.enabled);
        writer.writeBool("include_central_body", object.includeCentralBody);
        writer.writeUint32("particle_count", object.particleCount);
        writer.writeUint32("seed", object.seed);
        writer.writeFloat("mass", object.mass);
        writer.writeFloat("size", object.size);
        writer.writeFloat("radius_min", object.radiusMin);
        writer.writeFloat("radius_max", object.radiusMax);
        writer.writeFloat("thickness", object.thickness);
        writer.writeFloat("velocity_scale", object.velocityScale);
        writer.writeFloat("speed", object.speed);
        writer.writeFloat("particle_mass", object.particleMass);
        writer.writeFloat("x", object.positionX);
        writer.writeFloat("y", object.positionY);
        writer.writeFloat("z", object.positionZ);
        writer.writeFloat("vx", object.velocityX);
        writer.writeFloat("vy", object.velocityY);
        writer.writeFloat("vz", object.velocityZ);
        writer.writeBool("asset", object.isAsset);
        std::string properties;
        for (std::size_t index = 0u; index < object.properties.size(); ++index) {
            if (index != 0u) {
                properties += ',';
            }
            properties += object.properties[index];
        }
        writer.writeQuotedString("properties", properties);
        writer.writeFloat("offset_x", object.offsetX);
        writer.writeFloat("offset_y", object.offsetY);
        writer.writeFloat("offset_z", object.offsetZ);
        writer.writeFloat("rotation_x", object.rotationX);
        writer.writeFloat("rotation_y", object.rotationY);
        writer.writeFloat("rotation_z", object.rotationZ);
        writer.writeString("copy_axis", object.axis);
        writer.writeUint32("rotation_copies", object.copies);
        writer.writeBool("mirror_x", object.mirrorX);
        writer.writeBool("mirror_y", object.mirrorY);
        writer.writeBool("mirror_z", object.mirrorZ);
        writer.writeString("pivot", object.pivot);
        writer.writeFloat("pivot_x", object.pivotX);
        writer.writeFloat("pivot_y", object.pivotY);
        writer.writeFloat("pivot_z", object.pivotZ);
        if (!object.assetId.empty()) {
            writer.writeQuotedString("asset_id", object.assetId);
        }
        if (object.type == "particle_system") {
            writer.writeString("distribution", object.distribution);
            writer.writeFloat("particle_size", object.particleSize);
            writer.writeFloat("particle_height", object.particleHeight);
            writer.writeFloat("particle_speed", object.particleSpeed);
            writer.writeQuotedString("emitter_object_id", object.emitterObjectId);
            writer.writeQuotedString("target_asset_id", object.targetAssetId);
        }
        writer.finish();
    }
}

void writeTransform(std::ostream& out, const SimulationConfig& config)
{
    DirectiveStreamWriter writer(out, "transform");
    writer.writeFloat("offset_x", config.sceneOffsetX);
    writer.writeFloat("offset_y", config.sceneOffsetY);
    writer.writeFloat("offset_z", config.sceneOffsetZ);
    writer.writeFloat("rotation_x", config.sceneRotationX);
    writer.writeFloat("rotation_y", config.sceneRotationY);
    writer.writeFloat("rotation_z", config.sceneRotationZ);
    writer.writeString("copy_axis", config.sceneCopyAxis);
    writer.writeUint32("rotation_copies", config.sceneRotationCopies);
    writer.writeBool("mirror_x", config.sceneMirrorX);
    writer.writeBool("mirror_y", config.sceneMirrorY);
    writer.writeBool("mirror_z", config.sceneMirrorZ);
    writer.finish();
}
} // namespace bltzr_config
