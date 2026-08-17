/*
 * @file engine/server/src/simulation/state/Transforms.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Initial-state rotation, mirror, copy, and offset transforms.
 */

#include "simulation/Internal.hpp"
#include "Constants.hpp"

#include <algorithm>
#include <cmath>

static Vector3 rotateAroundAxis(const Vector3& value, std::string_view axis, float angle)
{
    const float sine = std::sin(angle);
    const float cosine = std::cos(angle);
    if (axis == "x") {
        return Vector3(value.x, cosine * value.y - sine * value.z,
                       sine * value.y + cosine * value.z);
    }
    if (axis == "y") {
        return Vector3(cosine * value.x + sine * value.z, value.y,
                       -sine * value.x + cosine * value.z);
    }
    return Vector3(cosine * value.x - sine * value.y,
                   sine * value.x + cosine * value.y, value.z);
}

static Vector3 rotateSceneEuler(const Vector3& value, const InitialStateConfig& config)
{
    Vector3 rotated = rotateAroundAxis(value, "x", config.sceneRotationX * kDegreesToRadians);
    rotated = rotateAroundAxis(rotated, "y", config.sceneRotationY * kDegreesToRadians);
    return rotateAroundAxis(rotated, "z", config.sceneRotationZ * kDegreesToRadians);
}

bool applyInitialStateTransform(std::vector<Particle>& particles,
                                const InitialStateConfig& config)
{
    if (particles.empty()) {
        return false;
    }
    const std::uint32_t copies = std::clamp(config.sceneRotationCopies, 1u, 256u);
    if (copies == 1u && !config.sceneMirrorX && !config.sceneMirrorY &&
        !config.sceneMirrorZ && config.sceneOffsetX == 0.0f && config.sceneOffsetY == 0.0f &&
        config.sceneOffsetZ == 0.0f && config.sceneRotationX == 0.0f &&
        config.sceneRotationY == 0.0f && config.sceneRotationZ == 0.0f) {
        return false;
    }
    std::uint32_t mirrorBit = 1u;
    const std::uint32_t xBit = config.sceneMirrorX ? mirrorBit++ : 0u;
    const std::uint32_t yBit = config.sceneMirrorY ? mirrorBit++ : 0u;
    const std::uint32_t zBit = config.sceneMirrorZ ? mirrorBit++ : 0u;
    const std::uint32_t mirrorVariants = 1u << (mirrorBit - 1u);
    const std::size_t outputCount = particles.size() * static_cast<std::size_t>(mirrorVariants) *
                                    static_cast<std::size_t>(copies);
    const std::vector<Particle> source = particles;
    std::vector<Particle> transformed;
    transformed.reserve(outputCount);
    const Vector3 pivot(config.scenePivotX, config.scenePivotY, config.scenePivotZ);
    for (std::uint32_t mirrorMask = 0u; mirrorMask < mirrorVariants; ++mirrorMask) {
        const bool flipX = config.sceneMirrorX && ((mirrorMask & xBit) != 0u);
        const bool flipY = config.sceneMirrorY && ((mirrorMask & yBit) != 0u);
        const bool flipZ = config.sceneMirrorZ && ((mirrorMask & zBit) != 0u);
        for (const Particle& original : source) {
            Vector3 position = original.getPosition() - pivot;
            Vector3 velocity = original.getVelocity();
            if (flipX) {
                position.x = -position.x;
                velocity.x = -velocity.x;
            }
            if (flipY) {
                position.y = -position.y;
                velocity.y = -velocity.y;
            }
            if (flipZ) {
                position.z = -position.z;
                velocity.z = -velocity.z;
            }
            position = rotateSceneEuler(position, config);
            velocity = rotateSceneEuler(velocity, config);
            for (std::uint32_t copy = 0u; copy < copies; ++copy) {
                const float angle = copies == 1u
                                        ? 0.0f
                                        : kTwoPi * static_cast<float>(copy) /
                                              static_cast<float>(copies);
                Particle duplicate = original;
                Vector3 copyPosition = rotateAroundAxis(position, config.sceneCopyAxis, angle);
                Vector3 copyVelocity = rotateAroundAxis(velocity, config.sceneCopyAxis, angle);
                copyPosition += pivot;
                copyPosition.x += config.sceneOffsetX;
                copyPosition.y += config.sceneOffsetY;
                copyPosition.z += config.sceneOffsetZ;
                duplicate.setPosition(copyPosition);
                duplicate.setVelocity(copyVelocity);
                transformed.push_back(duplicate);
            }
        }
    }
    particles.swap(transformed);
    return true;
}
