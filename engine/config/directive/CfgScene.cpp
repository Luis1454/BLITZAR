/*
 * @file engine/config/directive/CfgScene.cpp
 * @brief Scene object directive handling.
 */

#include "CfgDirectiveInternals.hpp"

#include "config/core/CfgConfig.hpp"

#include <algorithm>
#include <cctype>
#include <stdexcept>

namespace bltzr_config {
bool parseSceneFloat(const std::string& raw, float& target)
{
    try {
        std::size_t consumed = 0u;
        const float value = std::stof(raw, &consumed);
        if (consumed != raw.size()) {
            return false;
        }
        target = value;
        return true;
    }
    catch (const std::exception&) {
        return false;
    }
}

bool parseSceneUint(const std::string& raw, std::uint32_t& target)
{
    try {
        std::size_t consumed = 0u;
        const unsigned long value = std::stoul(raw, &consumed);
        if (consumed != raw.size() || value > 0xffffffffUL) {
            return false;
        }
        target = static_cast<std::uint32_t>(value);
        return true;
    }
    catch (const std::exception&) {
        return false;
    }
}

bool parseSceneBool(const std::string& raw, bool& target)
{
    std::string value = raw;
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    if (value == "true" || value == "1" || value == "on" || value == "yes") {
        target = true;
        return true;
    }
    if (value == "false" || value == "0" || value == "off" || value == "no") {
        target = false;
        return true;
    }
    return false;
}

static void addProperty(SceneObjectConfig& object, const std::string& property)
{
    if (property.empty() || std::find(object.properties.begin(), object.properties.end(),
                                      property) != object.properties.end()) {
        return;
    }
    object.properties.push_back(property);
}

static void addProperties(const std::string& raw, SceneObjectConfig& object)
{
    std::size_t begin = 0u;
    while (begin <= raw.size()) {
        const std::size_t end = raw.find(',', begin);
        addProperty(object,
                    raw.substr(begin, end == std::string::npos ? std::string::npos : end - begin));
        if (end == std::string::npos) {
            break;
        }
        begin = end + 1u;
    }
}

bool applySceneObjectArg(const DirectiveArgument& arg, SceneObjectConfig& object)
{
    const std::string& key = arg.first;
    const std::string& value = arg.second;
    if (key == "id")
        object.id = value;
    else if (key == "name")
        object.name = value;
    else if (key == "type")
        object.type = value;
    else if (key == "enabled")
        return parseSceneBool(value, object.enabled);
    else if (key == "include_central_body")
        return parseSceneBool(value, object.includeCentralBody);
    else if (key == "count" || key == "particle_count")
        return parseSceneUint(value, object.particleCount);
    else if (key == "seed")
        return parseSceneUint(value, object.seed);
    else if (key == "mass")
        return parseSceneFloat(value, object.mass);
    else if (key == "size")
        return parseSceneFloat(value, object.size);
    else if (key == "radius_min")
        return parseSceneFloat(value, object.radiusMin);
    else if (key == "radius_max")
        return parseSceneFloat(value, object.radiusMax);
    else if (key == "thickness")
        return parseSceneFloat(value, object.thickness);
    else if (key == "velocity_scale")
        return parseSceneFloat(value, object.velocityScale);
    else if (key == "speed")
        return parseSceneFloat(value, object.speed);
    else if (key == "particle_mass")
        return parseSceneFloat(value, object.particleMass);
    else if (key == "x")
        return parseSceneFloat(value, object.positionX);
    else if (key == "y")
        return parseSceneFloat(value, object.positionY);
    else if (key == "z")
        return parseSceneFloat(value, object.positionZ);
    else if (key == "vx")
        return parseSceneFloat(value, object.velocityX);
    else if (key == "vy")
        return parseSceneFloat(value, object.velocityY);
    else if (key == "vz")
        return parseSceneFloat(value, object.velocityZ);
    else if (key == "asset")
        return parseSceneBool(value, object.isAsset);
    else if (key == "asset_id")
        object.assetId = value;
    else if (key == "property")
        addProperty(object, value);
    else if (key == "properties")
        addProperties(value, object);
    else if (key == "offset_x")
        return parseSceneFloat(value, object.offsetX);
    else if (key == "offset_y")
        return parseSceneFloat(value, object.offsetY);
    else if (key == "offset_z")
        return parseSceneFloat(value, object.offsetZ);
    else if (key == "rotation_x")
        return parseSceneFloat(value, object.rotationX);
    else if (key == "rotation_y")
        return parseSceneFloat(value, object.rotationY);
    else if (key == "rotation_z")
        return parseSceneFloat(value, object.rotationZ);
    else if (key == "copy_axis" || key == "axis")
        object.axis = value;
    else if (key == "rotation_copies" || key == "copies")
        return parseSceneUint(value, object.copies);
    else if (key == "mirror_x")
        return parseSceneBool(value, object.mirrorX);
    else if (key == "mirror_y")
        return parseSceneBool(value, object.mirrorY);
    else if (key == "mirror_z")
        return parseSceneBool(value, object.mirrorZ);
    else if (key == "pivot")
        object.pivot = value;
    else if (key == "pivot_x")
        return parseSceneFloat(value, object.pivotX);
    else if (key == "pivot_y")
        return parseSceneFloat(value, object.pivotY);
    else if (key == "pivot_z")
        return parseSceneFloat(value, object.pivotZ);
    else if (key == "distribution")
        object.distribution = value;
    else if (key == "particle_size")
        return parseSceneFloat(value, object.particleSize);
    else if (key == "particle_height")
        return parseSceneFloat(value, object.particleHeight);
    else if (key == "particle_speed")
        return parseSceneFloat(value, object.particleSpeed);
    else if (key == "emitter_object_id")
        object.emitterObjectId = value;
    else if (key == "target_asset_id" || key == "instance_object_id")
        object.targetAssetId = value;
    else
        return false;
    return true;
}
} // namespace bltzr_config
