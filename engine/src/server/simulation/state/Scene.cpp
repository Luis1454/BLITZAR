/*
 * @file engine/src/server/simulation/state/Scene.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Scene object and particle-system state generation.
 */

#include "GenerationContext.hpp"
#include "Constants.hpp"

#include <algorithm>
#include <cmath>
#include <random>

static std::string sceneObjectMode(const SceneObjectConfig& object)
{
    const std::string type = toLower(object.type);
    if (type == "circle") return "disk_orbit";
    if (type == "cloud") return "random_cloud";
    if (type == "plummer" || type == "plummer_sphere") return "plummer_sphere";
    if (type == "galaxy") return "galaxy";
    if (type == "galaxy_collision") return "galaxy_collision";
    if (type == "binary" || type == "binary_star") return "two_body";
    if (type == "solar_system") return "solar_system";
    return type;
}

static bool hasSceneObjectProperty(const SceneObjectConfig& object, std::string_view property)
{
    return std::find(object.properties.begin(), object.properties.end(), property) !=
           object.properties.end();
}

static InitialStateConfig makeObjectInitialState(const InitialStateConfig& base,
                                                 const SceneObjectConfig& object)
{
    InitialStateConfig child = base;
    child.scene.objects.clear();
    child.mode = sceneObjectMode(object);
    child.seed = object.seed;
    child.includeCentralBody = object.includeCentralBody;
    child.centralMass = std::max(1e-6f, object.mass);
    child.centralX = object.positionX;
    child.centralY = object.positionY;
    child.centralZ = object.positionZ;
    child.centralVx = object.velocityX;
    child.centralVy = object.velocityY;
    child.centralVz = object.velocityZ;
    child.diskMass = std::max(1e-6f, object.mass);
    child.diskRadiusMin = std::max(0.01f, object.radiusMin);
    child.diskRadiusMax = std::max(child.diskRadiusMin + 0.0001f, object.radiusMax);
    child.diskThickness = std::max(0.0f, object.thickness);
    child.velocityScale = std::max(0.0f, object.velocityScale);
    child.cloudHalfExtent = std::max(0.1f, object.size);
    child.cubeHalfExtent = std::max(0.1f, object.size);
    child.sphereRadius = std::max(0.1f, object.size);
    child.cloudSpeed = std::max(0.0f, object.speed);
    child.particleMass = std::max(1e-8f, object.particleMass);
    const bool offset = hasSceneObjectProperty(object, "offset");
    const bool rotation = hasSceneObjectProperty(object, "rotation");
    const bool copies = hasSceneObjectProperty(object, "copies");
    const bool mirror = hasSceneObjectProperty(object, "mirror");
    const bool hasPivot = hasSceneObjectProperty(object, "pivot");
    child.sceneOffsetX = offset ? object.offsetX : 0.0f;
    child.sceneOffsetY = offset ? object.offsetY : 0.0f;
    child.sceneOffsetZ = offset ? object.offsetZ : 0.0f;
    const std::string pivot = hasPivot ? toLower(object.pivot) : "world";
    child.scenePivotX = pivot == "object" ? object.positionX : object.pivotX;
    child.scenePivotY = pivot == "object" ? object.positionY : object.pivotY;
    child.scenePivotZ = pivot == "object" ? object.positionZ : object.pivotZ;
    if (pivot == "world") {
        child.scenePivotX = 0.0f;
        child.scenePivotY = 0.0f;
        child.scenePivotZ = 0.0f;
    }
    child.sceneRotationX = rotation ? object.rotationX : 0.0f;
    child.sceneRotationY = rotation ? object.rotationY : 0.0f;
    child.sceneRotationZ = rotation ? object.rotationZ : 0.0f;
    child.sceneCopyAxis = copies ? object.axis : "z";
    child.sceneRotationCopies = copies ? std::clamp(object.copies, 1u, 256u) : 1u;
    child.sceneMirrorX = mirror && object.mirrorX;
    child.sceneMirrorY = mirror && object.mirrorY;
    child.sceneMirrorZ = mirror && object.mirrorZ;
    return child;
}

static Vector3 randomDiskPoint(std::mt19937& rng, float radius)
{
    std::uniform_real_distribution<float> unit(0.0f, 1.0f);
    const float angle = kTwoPi * unit(rng);
    const float distance = std::sqrt(unit(rng)) * radius;
    return Vector3(std::cos(angle) * distance, std::sin(angle) * distance, 0.0f);
}

static const SceneObjectConfig* findSceneObject(const SceneConfig& scene, std::string_view id)
{
    if (id.empty()) {
        return nullptr;
    }
    for (const SceneObjectConfig& object : scene.objects) {
        if (object.id == id) {
            return &object;
        }
    }
    return nullptr;
}

static bool buildSceneObjectBase(std::vector<Particle>& particles,
                                 const SceneObjectConfig& object,
                                 const InitialStateConfig& config)
{
    particles.clear();
    if (toLower(object.type) == "point") {
        Particle point;
        point.setMass(std::max(1e-6f, object.particleMass));
        point.setPosition(Vector3(object.positionX, object.positionY, object.positionZ));
        point.setVelocity(Vector3(object.velocityX, object.velocityY, object.velocityZ));
        point.setPressure(Vector3(0.0f, 0.0f, 0.0f));
        point.setDensity(0.0f);
        point.setTemperature(config.particleTemperature);
        particles.push_back(point);
        return true;
    }
    const InitialStateConfig child = makeObjectInitialState(config, object);
    return buildGeneratedStateSingle(particles, std::max<std::uint32_t>(2u, object.particleCount),
                                     child);
}

static bool appendReferencedObjectSystem(std::vector<Particle>& particles,
                                         const SceneObjectConfig& property,
                                         const SceneConfig& scene,
                                         const InitialStateConfig& config)
{
    const SceneObjectConfig* emitter = findSceneObject(scene, property.emitterObjectId);
    const SceneObjectConfig* instance = findSceneObject(scene, property.targetAssetId);
    if (emitter == nullptr || instance == nullptr || property.particleCount == 0u) {
        return false;
    }

    std::vector<Particle> emitterParticles;
    if (!buildSceneObjectBase(emitterParticles, *emitter, config) || emitterParticles.empty()) {
        return false;
    }
    particles.reserve(particles.size() + static_cast<std::size_t>(property.particleCount) *
                      static_cast<std::size_t>(std::max<std::uint32_t>(2u,
                                                                        instance->particleCount)));
    for (std::uint32_t index = 0u; index < property.particleCount; ++index) {
        const std::uint64_t scaledIndex =
            static_cast<std::uint64_t>(index) * emitterParticles.size();
        const std::size_t emitterIndex = static_cast<std::size_t>(
            (scaledIndex / property.particleCount) % emitterParticles.size());
        const Particle& emitterParticle = emitterParticles[emitterIndex];
        SceneObjectConfig instanceObject = *instance;
        const Vector3 center = emitterParticle.getPosition();
        const Vector3 centerVelocity = emitterParticle.getVelocity();
        instanceObject.positionX = center.x;
        instanceObject.positionY = center.y;
        instanceObject.positionZ = center.z;
        instanceObject.velocityX = centerVelocity.x;
        instanceObject.velocityY = centerVelocity.y;
        instanceObject.velocityZ = centerVelocity.z;
        instanceObject.seed += index * 0x9e3779b9u;
        std::vector<Particle> generated;
        if (!buildSceneObjectBase(generated, instanceObject, config)) {
            continue;
        }
        (void)applyInitialStateTransform(generated,
                                         makeObjectInitialState(config, instanceObject));
        particles.insert(particles.end(), generated.begin(), generated.end());
    }
    return true;
}

static void appendParticleSystem(std::vector<Particle>& particles,
                                 const SceneObjectConfig& property,
                                 const SceneConfig& scene,
                                 const InitialStateConfig& config)
{
    const std::uint32_t count = property.particleCount;
    if (count == 0u || appendReferencedObjectSystem(particles, property, scene, config)) {
        return;
    }
    if (particles.empty()) {
        return;
    }

    Vector3 position(0.0f, 0.0f, 0.0f);
    Vector3 velocity(0.0f, 0.0f, 0.0f);
    for (const Particle& particle : particles) {
        position += particle.getPosition();
        velocity += particle.getVelocity();
    }
    const float sourceCount = static_cast<float>(particles.size());
    position = position / sourceCount;
    velocity = velocity / sourceCount;
    const float size = std::max(0.0001f, std::abs(property.particleSize));
    const float height = std::max(0.0001f, std::abs(property.particleHeight));
    const float mass = std::max(1.0e-8f, std::abs(property.particleMass));
    const float speed = std::max(0.0f, std::abs(property.particleSpeed));
    const std::string distribution = toLower(property.distribution);
    std::mt19937 rng(property.seed);
    std::uniform_real_distribution<float> unit(-1.0f, 1.0f);
    std::uniform_real_distribution<float> positive(0.0f, 1.0f);
    std::vector<Vector3> treeCenters;
    if (distribution == "forest") {
        const std::uint32_t treeCount = std::max<std::uint32_t>(
            1u, std::min(count, static_cast<std::uint32_t>(std::ceil(
                    std::sqrt(static_cast<float>(count) / 24.0f)))));
        treeCenters.reserve(treeCount);
        for (std::uint32_t tree = 0u; tree < treeCount; ++tree) {
            treeCenters.push_back(randomDiskPoint(rng, size));
        }
    }
    particles.reserve(particles.size() + static_cast<std::size_t>(count));
    for (std::uint32_t index = 0u; index < count; ++index) {
        Vector3 local(0.0f, 0.0f, 0.0f);
        if (distribution == "forest") {
            const Vector3 center = treeCenters[index % treeCenters.size()];
            local = center + Vector3(unit(rng) * size * 0.08f, unit(rng) * size * 0.08f,
                                     positive(rng) * height);
        }
        else if (distribution == "uniform_box") {
            local = Vector3(unit(rng) * size, unit(rng) * size, unit(rng) * size);
        }
        else {
            for (std::uint32_t attempt = 0u; attempt < 16u; ++attempt) {
                local = Vector3(unit(rng) * size, unit(rng) * size, unit(rng) * size);
                if (local.x * local.x + local.y * local.y + local.z * local.z <= size * size) {
                    break;
                }
            }
        }
        Particle particle;
        particle.setMass(mass);
        particle.setPosition(position + local);
        particle.setVelocity(velocity + Vector3(unit(rng) * speed, unit(rng) * speed,
                                                unit(rng) * speed));
        particle.setPressure(Vector3(0.0f, 0.0f, 0.0f));
        particle.setDensity(0.0f);
        particle.setTemperature(std::max(0.0f, config.particleTemperature));
        particles.push_back(particle);
    }
}

bool buildGeneratedState(std::vector<Particle>& outParticles, std::uint32_t particleCount,
                         const InitialStateConfig& config)
{
    if (config.scene.objects.empty()) {
        return buildGeneratedStateSingle(outParticles, particleCount, config);
    }
    outParticles.clear();
    for (const SceneObjectConfig& object : config.scene.objects) {
        if (!object.enabled) {
            continue;
        }
        if (toLower(object.type) == "particle_system") {
            std::vector<Particle> source;
            const SceneObjectConfig* emitter = findSceneObject(config.scene, object.emitterObjectId);
            if (emitter != nullptr) {
                (void)buildSceneObjectBase(source, *emitter, config);
            }
            if (source.empty()) {
                Particle anchor;
                anchor.setMass(std::max(1.0e-6f, object.particleMass));
                anchor.setPosition(Vector3(object.positionX, object.positionY, object.positionZ));
                anchor.setVelocity(Vector3(object.velocityX, object.velocityY, object.velocityZ));
                source.push_back(anchor);
            }
            const std::size_t sourceCount = source.size();
            appendParticleSystem(source, object, config.scene, config);
            if (source.size() > sourceCount) {
                source.erase(source.begin(), source.begin() + static_cast<std::ptrdiff_t>(sourceCount));
                (void)applyInitialStateTransform(source, makeObjectInitialState(config, object));
                outParticles.insert(outParticles.end(), source.begin(), source.end());
            }
            continue;
        }
        std::vector<Particle> objectParticles;
        if (!buildSceneObjectBase(objectParticles, object, config)) {
            continue;
        }
        const std::size_t baseCount = objectParticles.size();
        if (hasSceneObjectProperty(object, "particle_system")) {
            appendParticleSystem(objectParticles, object, config.scene, config);
            if (objectParticles.size() > baseCount) {
                std::vector<Particle> generated(
                    objectParticles.begin() + static_cast<std::ptrdiff_t>(baseCount),
                    objectParticles.end());
                (void)applyInitialStateTransform(generated, makeObjectInitialState(config, object));
                outParticles.insert(outParticles.end(), generated.begin(), generated.end());
                objectParticles.resize(baseCount);
            }
        }
        (void)applyInitialStateTransform(objectParticles, makeObjectInitialState(config, object));
        outParticles.insert(outParticles.end(), objectParticles.begin(), objectParticles.end());
    }
    return outParticles.size() >= 2u;
}
