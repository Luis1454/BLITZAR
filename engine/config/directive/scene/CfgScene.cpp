/*
 * @file engine/config/directive/scene/CfgScene.cpp
 * @brief Scene object directive handling.
 */

#include "config/directive/parsing/CfgDirectiveInternals.hpp"

#include "config/core/configuration/CfgConfig.hpp"

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

enum class SceneArgResult { unhandled, applied, invalid };

static SceneArgResult applySceneIdentityArg(const std::string& key, const std::string& value,
                                            SceneObjectConfig& object)
{
    if (key == "id")
        object.id = value;
    else if (key == "name")
        object.name = value;
    else if (key == "type")
        object.type = value;
    else if (key == "asset_id")
        object.assetId = value;
    else if (key == "emitter_object_id")
        object.emitterObjectId = value;
    else if (key == "target_asset_id" || key == "instance_object_id")
        object.targetAssetId = value;
    else
        return SceneArgResult::unhandled;
    return SceneArgResult::applied;
}

static SceneArgResult applySceneScalarArg(const std::string& key, const std::string& value,
                                          SceneObjectConfig& object)
{
    if (key == "enabled")
        return parseSceneBool(value, object.enabled) ? SceneArgResult::applied
                                                     : SceneArgResult::invalid;
    if (key == "include_central_body")
        return parseSceneBool(value, object.includeCentralBody) ? SceneArgResult::applied
                                                                 : SceneArgResult::invalid;
    if (key == "count" || key == "particle_count")
        return parseSceneUint(value, object.particleCount) ? SceneArgResult::applied
                                                           : SceneArgResult::invalid;
    if (key == "seed")
        return parseSceneUint(value, object.seed) ? SceneArgResult::applied
                                                  : SceneArgResult::invalid;
    if (key == "mass")
        return parseSceneFloat(value, object.mass) ? SceneArgResult::applied
                                                   : SceneArgResult::invalid;
    if (key == "size")
        return parseSceneFloat(value, object.size) ? SceneArgResult::applied
                                                   : SceneArgResult::invalid;
    if (key == "radius_min")
        return parseSceneFloat(value, object.radiusMin) ? SceneArgResult::applied
                                                        : SceneArgResult::invalid;
    if (key == "radius_max")
        return parseSceneFloat(value, object.radiusMax) ? SceneArgResult::applied
                                                        : SceneArgResult::invalid;
    if (key == "thickness")
        return parseSceneFloat(value, object.thickness) ? SceneArgResult::applied
                                                        : SceneArgResult::invalid;
    if (key == "velocity_scale")
        return parseSceneFloat(value, object.velocityScale) ? SceneArgResult::applied
                                                            : SceneArgResult::invalid;
    if (key == "speed")
        return parseSceneFloat(value, object.speed) ? SceneArgResult::applied
                                                    : SceneArgResult::invalid;
    if (key == "particle_mass")
        return parseSceneFloat(value, object.particleMass) ? SceneArgResult::applied
                                                           : SceneArgResult::invalid;
    return SceneArgResult::unhandled;
}

static SceneArgResult applySceneVectorArg(const std::string& key, const std::string& value,
                                          SceneObjectConfig& object)
{
    if (key == "x")
        return parseSceneFloat(value, object.positionX) ? SceneArgResult::applied
                                                        : SceneArgResult::invalid;
    if (key == "y")
        return parseSceneFloat(value, object.positionY) ? SceneArgResult::applied
                                                        : SceneArgResult::invalid;
    if (key == "z")
        return parseSceneFloat(value, object.positionZ) ? SceneArgResult::applied
                                                        : SceneArgResult::invalid;
    if (key == "vx")
        return parseSceneFloat(value, object.velocityX) ? SceneArgResult::applied
                                                        : SceneArgResult::invalid;
    if (key == "vy")
        return parseSceneFloat(value, object.velocityY) ? SceneArgResult::applied
                                                        : SceneArgResult::invalid;
    if (key == "vz")
        return parseSceneFloat(value, object.velocityZ) ? SceneArgResult::applied
                                                        : SceneArgResult::invalid;
    if (key == "offset_x")
        return parseSceneFloat(value, object.offsetX) ? SceneArgResult::applied
                                                      : SceneArgResult::invalid;
    if (key == "offset_y")
        return parseSceneFloat(value, object.offsetY) ? SceneArgResult::applied
                                                      : SceneArgResult::invalid;
    if (key == "offset_z")
        return parseSceneFloat(value, object.offsetZ) ? SceneArgResult::applied
                                                      : SceneArgResult::invalid;
    return SceneArgResult::unhandled;
}

static SceneArgResult applySceneTransformArg(const std::string& key, const std::string& value,
                                             SceneObjectConfig& object)
{
    if (key == "rotation_x")
        return parseSceneFloat(value, object.rotationX) ? SceneArgResult::applied
                                                        : SceneArgResult::invalid;
    if (key == "rotation_y")
        return parseSceneFloat(value, object.rotationY) ? SceneArgResult::applied
                                                        : SceneArgResult::invalid;
    if (key == "rotation_z")
        return parseSceneFloat(value, object.rotationZ) ? SceneArgResult::applied
                                                        : SceneArgResult::invalid;
    if (key == "copy_axis" || key == "axis")
        object.axis = value;
    else if (key == "rotation_copies" || key == "copies")
        return parseSceneUint(value, object.copies) ? SceneArgResult::applied
                                                    : SceneArgResult::invalid;
    else if (key == "mirror_x")
        return parseSceneBool(value, object.mirrorX) ? SceneArgResult::applied
                                                     : SceneArgResult::invalid;
    else if (key == "mirror_y")
        return parseSceneBool(value, object.mirrorY) ? SceneArgResult::applied
                                                     : SceneArgResult::invalid;
    else if (key == "mirror_z")
        return parseSceneBool(value, object.mirrorZ) ? SceneArgResult::applied
                                                     : SceneArgResult::invalid;
    else if (key == "pivot")
        object.pivot = value;
    else if (key == "pivot_x")
        return parseSceneFloat(value, object.pivotX) ? SceneArgResult::applied
                                                     : SceneArgResult::invalid;
    else if (key == "pivot_y")
        return parseSceneFloat(value, object.pivotY) ? SceneArgResult::applied
                                                     : SceneArgResult::invalid;
    else if (key == "pivot_z")
        return parseSceneFloat(value, object.pivotZ) ? SceneArgResult::applied
                                                     : SceneArgResult::invalid;
    else
        return SceneArgResult::unhandled;
    return SceneArgResult::applied;
}

static SceneArgResult applySceneParticleArg(const std::string& key, const std::string& value,
                                            SceneObjectConfig& object)
{
    if (key == "property")
        addProperty(object, value);
    else if (key == "properties")
        addProperties(value, object);
    else if (key == "asset")
        return parseSceneBool(value, object.isAsset) ? SceneArgResult::applied
                                                     : SceneArgResult::invalid;
    else if (key == "distribution")
        object.distribution = value;
    else if (key == "particle_size")
        return parseSceneFloat(value, object.particleSize) ? SceneArgResult::applied
                                                           : SceneArgResult::invalid;
    else if (key == "particle_height")
        return parseSceneFloat(value, object.particleHeight) ? SceneArgResult::applied
                                                             : SceneArgResult::invalid;
    else if (key == "particle_speed")
        return parseSceneFloat(value, object.particleSpeed) ? SceneArgResult::applied
                                                            : SceneArgResult::invalid;
    else
        return SceneArgResult::unhandled;
    return SceneArgResult::applied;
}

bool applySceneObjectArg(const DirectiveArgument& arg, SceneObjectConfig& object)
{
    const std::string& key = arg.first;
    const std::string& value = arg.second;
    const SceneArgResult results[] = {applySceneIdentityArg(key, value, object),
                                      applySceneScalarArg(key, value, object),
                                      applySceneVectorArg(key, value, object),
                                      applySceneTransformArg(key, value, object),
                                      applySceneParticleArg(key, value, object)};
    for (const SceneArgResult result : results) {
        if (result == SceneArgResult::applied)
            return true;
        if (result == SceneArgResult::invalid)
            return false;
    }
    return false;
}
} // namespace bltzr_config
