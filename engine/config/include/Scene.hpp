/*
 * @file engine/config/include/Scene.hpp
 * @brief Persistent scene object, transform and asset contracts.
 */

#ifndef BLITZAR_ENGINE_INCLUDE_CONFIG_SCENE_HPP_
#define BLITZAR_ENGINE_INCLUDE_CONFIG_SCENE_HPP_

#include <cstdint>
#include <string>
#include <vector>

struct SceneObjectConfig {
    std::string id;
    std::string name = "Object";
    std::string type = "galaxy";
    bool enabled = true;
    bool includeCentralBody = true;
    std::uint32_t particleCount = 10000u;
    std::uint32_t seed = 42u;
    float mass = 0.75f;
    float size = 12.0f;
    float radiusMin = 1.5f;
    float radiusMax = 11.5f;
    float thickness = 0.0f;
    float velocityScale = 1.0f;
    float speed = 0.0f;
    float particleMass = 0.01f;
    float positionX = 0.0f;
    float positionY = 0.0f;
    float positionZ = 0.0f;
    float velocityX = 0.0f;
    float velocityY = 0.0f;
    float velocityZ = 0.0f;
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
    bool isAsset = false;
    std::string assetId;
    std::string distribution = "uniform_sphere";
    float particleSize = 1.0f;
    float particleHeight = 1.0f;
    float particleSpeed = 0.0f;
    std::string emitterObjectId;
    std::string targetAssetId;
    // Optional properties owned by this object and edited from its scene card.
    std::vector<std::string> properties;
};

struct SceneConfig {
    std::vector<SceneObjectConfig> objects;
};

#endif // BLITZAR_ENGINE_INCLUDE_CONFIG_SCENE_HPP_
