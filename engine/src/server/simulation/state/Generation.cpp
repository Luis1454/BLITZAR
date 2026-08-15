/*
 * @file engine/src/server/simulation/state/Generation.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Source artifact for the BLITZAR simulation project.
 */

#include "engine/src/server/simulation/Internal.hpp"
#include "engine/src/server/simulation/state/InitializationHelper.hpp"
#include "Constants.hpp"
#include <cstddef>
#include <cmath>
#include <cstdio>
#include <random>
#include <omp.h>

static float squaredLength(Vector3 value)
{
    return value.x * value.x + value.y * value.y + value.z * value.z;
}

/*
 * @brief Documents the build generated state operation contract.
 * @param outParticles Input value used by this contract.
 * @param particleCount Input value used by this contract.
 * @param config Input value used by this contract.
 * @return bool value produced by this contract.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
static bool buildGeneratedStateSingle(std::vector<Particle>& outParticles,
                                      std::uint32_t particleCount,
                                      const InitialStateConfig& config)
{
    outParticles.clear();
    const std::uint32_t count = std::max<std::uint32_t>(2u, particleCount);
    const std::string mode = toLower(config.mode);
    std::mt19937 rng(config.seed);
    const float centralMass = std::max(1e-6f, config.centralMass);
    const float velocityTemperature = std::max(0.0f, config.velocityTemperature);
    const float particleTemperature = std::max(0.0f, config.particleTemperature);
    const Vector3 centralPos(config.centralX, config.centralY, config.centralZ);
    const Vector3 centralVel(config.centralVx, config.centralVy, config.centralVz);
    auto applyThermalVelocity = [&](Particle& p) {
        if (velocityTemperature <= 0.0f) {
            return;
        }
        float sigma = std::sqrt(velocityTemperature) * 0.005f;
        sigma = std::min(sigma, 0.1f);
        if (sigma <= 0.0f) {
            return;
        }
        std::normal_distribution<float> thermalDist(0.0f, sigma);
        const Vector3 v = p.getVelocity();
        p.setVelocity(
            Vector3(v.x + thermalDist(rng), v.y + thermalDist(rng), v.z + thermalDist(rng)));
    };
    auto addCentralBody = [&]() {
        if (!config.includeCentralBody) {
            return;
        }
        Particle central;
        central.setMass(centralMass);
        central.setPosition(centralPos);
        central.setVelocity(centralVel);
        central.setPressure(Vector3(0.0f, 0.0f, 0.0f));
        central.setDensity(0.0f);
        central.setTemperature(particleTemperature);
        outParticles.push_back(central);
    };
    auto finalizeParticle = [&](Particle& particle) {
        applyThermalVelocity(particle);
        particle.setPressure(Vector3(0.0f, 0.0f, 0.0f));
        particle.setDensity(0.0f);
        particle.setTemperature(particleTemperature);
    };
    if (mode == "two_body" || mode == "binary_star") {
        const float separation = std::max(0.2f, config.cloudHalfExtent);
        const float mass = std::max(1e-6f, config.particleMass);
        const float radius = 0.5f * separation;
        const float orbitalSpeed = std::sqrt(mass / std::max(2.0f * separation, 1e-6f)) *
                                   std::max(0.0f, config.velocityScale);
        Particle left;
        left.setMass(mass);
        left.setPosition(centralPos + Vector3(-radius, 0.0f, 0.0f));
        left.setVelocity(centralVel + Vector3(0.0f, -orbitalSpeed, 0.0f));
        finalizeParticle(left);
        outParticles.push_back(left);
        Particle right;
        right.setMass(mass);
        right.setPosition(centralPos + Vector3(radius, 0.0f, 0.0f));
        right.setVelocity(centralVel + Vector3(0.0f, orbitalSpeed, 0.0f));
        finalizeParticle(right);
        outParticles.push_back(right);
        return true;
    }
    if (mode == "three_body") {
        const float scale = std::max(0.1f, config.cloudHalfExtent);
        const float mass = std::max(1e-6f, config.particleMass);
        const float speedScale = std::max(0.0f, config.velocityScale) / std::sqrt(scale);
        constexpr float kX = 0.97000436f;
        constexpr float kY = 0.24308753f;
        constexpr float kVx = 0.46620368f;
        constexpr float kVy = 0.43236572f;
        const Vector3 positions[] = {Vector3(-kX * scale, kY * scale, 0.0f),
                                     Vector3(kX * scale, -kY * scale, 0.0f),
                                     Vector3(0.0f, 0.0f, 0.0f)};
        const Vector3 velocities[] = {
            Vector3(kVx * speedScale, kVy * speedScale, 0.0f),
            Vector3(kVx * speedScale, kVy * speedScale, 0.0f),
            Vector3(-2.0f * kVx * speedScale, -2.0f * kVy * speedScale, 0.0f)};
        for (int index = 0; index < 3; index += 1) {
            Particle particle;
            particle.setMass(mass);
            particle.setPosition(centralPos + positions[index]);
            particle.setVelocity(centralVel + velocities[index]);
            finalizeParticle(particle);
            outParticles.push_back(particle);
        }
        return true;
    }
    if (mode == "plummer_sphere") {
        const float scale = std::max(0.1f, config.cloudHalfExtent);
        const float totalMass = std::max(1e-6f, config.particleMass * static_cast<float>(count));
        const float mass = std::max(1e-6f, totalMass / static_cast<float>(count));
        const float sigma = std::sqrt(totalMass / std::max(6.0f * scale, 1e-6f)) *
                            std::max(0.0f, config.velocityScale);
        std::uniform_real_distribution<float> unitDist(1e-4f, 0.9999f);
        std::uniform_real_distribution<float> azimuthDist(0.0f, kTwoPi);
        std::uniform_real_distribution<float> cosThetaDist(-1.0f, 1.0f);
        std::normal_distribution<float> velDist(0.0f, sigma);

        // Pre-generate with margin to handle rejection; sequential for determinism
        RandomData plummerData;
        constexpr float kAcceptanceFactor = 1.2f;  // 20% margin for rejection sampling
        std::size_t targetGenerated = static_cast<std::size_t>(count * kAcceptanceFactor);
        plummerData.reserve(targetGenerated, 9);  // u, azi, cosTheta, vx, vy, vz, rx, ry, rz

        while (plummerData.particleCount() < targetGenerated) {
            const float u = unitDist(rng);
            const float azimuth = azimuthDist(rng);
            const float cosTheta = cosThetaDist(rng);
            plummerData.push(u);
            plummerData.push(azimuth);
            plummerData.push(cosTheta);
            plummerData.push(velDist(rng));
            plummerData.push(velDist(rng));
            plummerData.push(velDist(rng));
            const float sinTheta = std::sqrt(std::max(0.0f, 1.0f - cosTheta * cosTheta));
            const float radius = scale / std::sqrt(std::pow(u, -2.0f / 3.0f) - 1.0f);
            plummerData.push(radius * sinTheta * std::cos(azimuth));
            plummerData.push(radius * sinTheta * std::sin(azimuth));
            plummerData.push(radius * cosTheta);
        }

        // Parallel particle fabrication (generate all, then trim)
        std::vector<Particle> generatedParticles;
        generatedParticles.reserve(std::min(static_cast<std::size_t>(plummerData.particleCount()), static_cast<std::size_t>(count) * 2u));
        Vector3 meanPosition(0.0f, 0.0f, 0.0f);
        Vector3 meanVelocity(0.0f, 0.0f, 0.0f);

#pragma omp parallel if (!config.deterministicMode)
        {
            std::vector<Particle> threadParticles;
            Vector3 threadMeanPos(0.0f, 0.0f, 0.0f);
            Vector3 threadMeanVel(0.0f, 0.0f, 0.0f);
            const std::ptrdiff_t particleTotal =
                static_cast<std::ptrdiff_t>(plummerData.particleCount());

#pragma omp for nowait
            for (std::ptrdiff_t i = 0; i < particleTotal; ++i) {
                const std::size_t particleIndex = static_cast<std::size_t>(i);
                Particle particle;
                particle.setMass(mass);
                const Vector3 offset(plummerData.get(particleIndex, 6),
                                     plummerData.get(particleIndex, 7),
                                     plummerData.get(particleIndex, 8));
                particle.setPosition(centralPos + offset);
                particle.setVelocity(centralVel + Vector3(plummerData.get(particleIndex, 3),
                                                           plummerData.get(particleIndex, 4),
                                                           plummerData.get(particleIndex, 5)));
                finalizeParticle(particle);
                threadParticles.push_back(particle);
                threadMeanPos = threadMeanPos + particle.getPosition();
                threadMeanVel = threadMeanVel + particle.getVelocity();
            }

#pragma omp critical
            {
                generatedParticles.insert(generatedParticles.end(), threadParticles.begin(), threadParticles.end());
                meanPosition = meanPosition + threadMeanPos;
                meanVelocity = meanVelocity + threadMeanVel;
            }
        }

        // Trim to exact count and recenter
        if (generatedParticles.size() > count) {
            generatedParticles.resize(count);
        }
        const float invCount = 1.0f / static_cast<float>(generatedParticles.size());
        meanPosition = meanPosition * invCount;
        meanVelocity = meanVelocity * invCount;
        for (Particle& particle : generatedParticles) {
            particle.setPosition(particle.getPosition() - meanPosition + centralPos);
            particle.setVelocity(particle.getVelocity() - meanVelocity + centralVel);
        }
        outParticles.insert(outParticles.end(), generatedParticles.begin(), generatedParticles.end());
        return outParticles.size() >= 2;
    }
    if (mode == "random_cloud") {
        const float halfExtent = std::max(0.01f, config.cloudHalfExtent);
        const float cloudSpeed = std::max(0.0f, config.cloudSpeed);
        const float particleMass = std::max(1e-6f, config.particleMass);
        std::uniform_real_distribution<float> posDist(-halfExtent, halfExtent);
        std::uniform_real_distribution<float> velDist(-cloudSpeed, cloudSpeed);
        addCentralBody();

        // Pre-generate random values sequentially for determinism, then parallelize particle creation
        RandomData randomValues;
        randomValues.reserve(count, 6);  // 6 floats per particle: px, py, pz, vx, vy, vz
        while (randomValues.particleCount() < count) {
            randomValues.push(posDist(rng));
            randomValues.push(posDist(rng));
            randomValues.push(posDist(rng));
            randomValues.push(velDist(rng));
            randomValues.push(velDist(rng));
            randomValues.push(velDist(rng));
        }

        // Parallel particle fabrication from pre-generated data
        outParticles.reserve(randomValues.particleCount());
#pragma omp parallel if (!config.deterministicMode)
        {
            std::vector<Particle> threadParticles;
            const std::ptrdiff_t particleTotal =
                static_cast<std::ptrdiff_t>(randomValues.particleCount());
#pragma omp for nowait
            for (std::ptrdiff_t i = 0; i < particleTotal; ++i) {
                const std::size_t particleIndex = static_cast<std::size_t>(i);
                Particle p;
                p.setMass(particleMass);
                p.setPosition(Vector3(centralPos.x + randomValues.get(particleIndex, 0),
                                      centralPos.y + randomValues.get(particleIndex, 1),
                                      centralPos.z + randomValues.get(particleIndex, 2)));
                p.setVelocity(Vector3(centralVel.x + randomValues.get(particleIndex, 3),
                                      centralVel.y + randomValues.get(particleIndex, 4),
                                      centralVel.z + randomValues.get(particleIndex, 5)));
                finalizeParticle(p);
                threadParticles.push_back(p);
            }
#pragma omp critical
            outParticles.insert(outParticles.end(), threadParticles.begin(), threadParticles.end());
        }
        return outParticles.size() >= 2;
    }
    if (mode == "cube_random") {
        const float halfExtent = std::max(0.01f, config.cubeHalfExtent);
        const float cloudSpeed = std::max(0.0f, config.cloudSpeed);
        const float particleMass = std::max(1e-6f, config.particleMass);
        std::uniform_real_distribution<float> posDist(-halfExtent, halfExtent);
        std::uniform_real_distribution<float> velDist(-cloudSpeed, cloudSpeed);
        addCentralBody();
        while (outParticles.size() < count) {
            Particle p;
            p.setMass(particleMass);
            p.setPosition(Vector3(centralPos.x + posDist(rng), centralPos.y + posDist(rng),
                                  centralPos.z + posDist(rng)));
            p.setVelocity(Vector3(centralVel.x + velDist(rng), centralVel.y + velDist(rng),
                                  centralVel.z + velDist(rng)));
            finalizeParticle(p);
            outParticles.push_back(p);
        }
        return outParticles.size() >= 2;
    }
    if (mode == "sphere_random") {
        const float radius = std::max(0.01f, config.sphereRadius);
        const float cloudSpeed = std::max(0.0f, config.cloudSpeed);
        const float particleMass = std::max(1e-6f, config.particleMass);
        const float radius2 = radius * radius;
        std::uniform_real_distribution<float> posDist(-radius, radius);
        std::uniform_real_distribution<float> velDist(-cloudSpeed, cloudSpeed);
        addCentralBody();
        while (outParticles.size() < count) {
            const float x = posDist(rng);
            const float y = posDist(rng);
            const float z = posDist(rng);
            if (x * x + y * y + z * z > radius2) {
                continue;
            }
            Particle p;
            p.setMass(particleMass);
            p.setPosition(Vector3(centralPos.x + x, centralPos.y + y, centralPos.z + z));
            p.setVelocity(Vector3(centralVel.x + velDist(rng), centralVel.y + velDist(rng),
                                  centralVel.z + velDist(rng)));
            finalizeParticle(p);
            outParticles.push_back(p);
        }
        return outParticles.size() >= 2;
    }
    if (mode == "cosmology") {
        const CosmologyConfig& cosmology = config.cosmology;
        const bool comoving = isComovingCosmology(cosmology);
        const float extent = std::max(0.01f, cosmology.boxHalfExtent);
        const float radius = std::max(0.01f, cosmology.sphereRadius);
        const float particleMass = resolveCosmologyParticleMass(
            cosmology, config.particleMass, std::max<std::uint32_t>(2u, particleCount));
        const float scaleFactor = std::max(1e-6f, cosmology.initialScaleFactor);
        const float omegaMatter = std::max(0.0f, cosmology.omegaMatter);
        const float omegaLambda = std::max(0.0f, cosmology.omegaLambda);
        const float omegaRadiation = std::max(0.0f, cosmology.omegaRadiation);
        const float hubbleSquared =
            cosmology.hubbleH0 * cosmology.hubbleH0 *
            (omegaRadiation / std::pow(scaleFactor, 4.0f) +
             omegaMatter / std::pow(scaleFactor, 3.0f) + omegaLambda);
        const float hubbleRate = std::sqrt(std::max(0.0f, hubbleSquared));
        const float totalMass = cosmologyReferenceTotalMass(
            cosmology, particleMass, std::max<std::uint32_t>(2u, particleCount));
        std::fprintf(stderr,
                     "[cosmology] mass_model=%s total_mass=%.9g particle_mass=%.9g\n",
                     cosmology.massModel.c_str(), totalMass, particleMass);
        const float perturbation = std::clamp(cosmology.perturbationAmplitude, 0.0f, 1.0f);
        const float displacementScale = perturbation * extent * 0.05f;
        const float waveNumber = kTwoPi / std::max(2.0f * extent, 0.01f);
        std::uniform_real_distribution<float> unit(-1.0f, 1.0f);

        outParticles.reserve(count);
        while (outParticles.size() < count) {
            Vector3 lagrangian;
            if (toLower(cosmology.geometry) == "cube") {
                lagrangian = Vector3(unit(rng) * extent, unit(rng) * extent, unit(rng) * extent);
            }
            else {
                lagrangian = Vector3(unit(rng) * radius, unit(rng) * radius, unit(rng) * radius);
                if (squaredLength(lagrangian) > radius * radius) {
                    continue;
                }
            }

            const Vector3 displacement(
                displacementScale * std::sin(waveNumber * lagrangian.x),
                displacementScale * std::sin(waveNumber * lagrangian.y),
                displacementScale * std::sin(waveNumber * lagrangian.z));
            const Vector3 relativePosition = lagrangian + displacement;
            const Vector3 peculiarVelocity =
                displacement * (hubbleRate * std::max(0.0f, cosmology.peculiarVelocityScale));
            Particle particle;
            particle.setMass(particleMass);
            if (comoving) {
                const float boxLength = 2.0f * extent;
                const auto wrap = [boxLength](float value) {
                    const float wrapped = std::fmod(value, boxLength);
                    return wrapped < 0.0f ? wrapped + boxLength : wrapped;
                };
                particle.setPosition(Vector3(wrap(relativePosition.x + extent),
                                             wrap(relativePosition.y + extent),
                                             wrap(relativePosition.z + extent)));
                // The generator produces physical peculiar velocity. The comoving solver stores
                // p=a*v_phys=a^2*dx/dt as its velocity register.
                particle.setVelocity(peculiarVelocity * scaleFactor);
            }
            else {
                particle.setPosition(centralPos + relativePosition);
                // The preview path applies the Hubble operator at runtime; retain only peculiar
                // velocity here to avoid applying the background expansion twice.
                particle.setVelocity(centralVel + peculiarVelocity);
            }
            finalizeParticle(particle);
            outParticles.push_back(particle);
        }
        return outParticles.size() >= 2;
    }
    if (mode == "galaxy" || mode == "galaxy_collision") {
        const float size = std::max(0.1f, config.cloudHalfExtent);
        const float radiusLimit = std::max(0.1f, size);
        const float configuredRadiusMax =
            std::max(0.01f, std::min(config.diskRadiusMax, radiusLimit));
        const float radiusMin =
            std::max(0.01f, std::min(config.diskRadiusMin, configuredRadiusMax));
        const float radiusMax = std::max(radiusMin + 0.0001f, configuredRadiusMax);
        const float radiusMin2 = radiusMin * radiusMin;
        const float radiusMax2 = radiusMax * radiusMax;
        const float radiusRange2 = std::max(1e-6f, radiusMax2 - radiusMin2);
        const float requestedParticleMass = std::max(1e-8f, config.particleMass);
        auto generateDisk = [&](std::uint32_t n, Vector3 center, Vector3 bulkVelocity,
                                float nominalMass, std::uint32_t seedOffset) {
            if (n == 0u) {
                return;
            }
            std::mt19937 diskRng(config.seed + seedOffset);
            const float massPerPart = std::min(
                requestedParticleMass, std::max(1e-8f, nominalMass / static_cast<float>(n)));
            const float actualDiskMass = massPerPart * static_cast<float>(n);
            const float softening = std::max(0.05f, radiusMin * 0.5f);
            const std::size_t firstParticle = outParticles.size();
            std::uniform_real_distribution<float> rDist(radiusMin, radiusMax);
            std::uniform_real_distribution<float> aDist(0.0f, kTwoPi);
            for (std::uint32_t i = 0; i < n; ++i) {
                const float r = rDist(diskRng);
                const float a = aDist(diskRng);
                const Vector3 pos = center + Vector3(r * std::cos(a), r * std::sin(a), 0.0f);
                const float fraction = std::max(
                    0.05f, std::clamp((r * r - radiusMin2) / radiusRange2, 0.0f, 1.0f));
                const float enclosedMass =
                    (config.includeCentralBody ? centralMass : 0.0f) + actualDiskMass * fraction;
                const float softenedRadius2 = r * r + softening * softening;
                const float acceleration = enclosedMass * r /
                                           std::pow(softenedRadius2, 1.5f);
                const float speed = std::sqrt(std::max(0.0f, acceleration * r)) *
                                    std::max(0.0f, config.velocityScale);
                const Vector3 tangent(-std::sin(a) * speed, std::cos(a) * speed, 0.0f);
                Particle p;
                p.setMass(massPerPart);
                p.setPosition(pos);
                p.setVelocity(bulkVelocity + tangent);
                finalizeParticle(p);
                outParticles.push_back(p);
            }

            Vector3 meanPosition(0.0f, 0.0f, 0.0f);
            Vector3 meanVelocity(0.0f, 0.0f, 0.0f);
            const float inverseCount = 1.0f / static_cast<float>(n);
            for (std::size_t index = firstParticle; index < outParticles.size(); ++index) {
                meanPosition = meanPosition + outParticles[index].getPosition();
                meanVelocity = meanVelocity + outParticles[index].getVelocity();
            }
            meanPosition = meanPosition * inverseCount;
            meanVelocity = meanVelocity * inverseCount;
            for (std::size_t index = firstParticle; index < outParticles.size(); ++index) {
                Particle& particle = outParticles[index];
                particle.setPosition(particle.getPosition() - meanPosition + center);
                particle.setVelocity(particle.getVelocity() - meanVelocity + bulkVelocity);
            }
        };

        if (mode == "galaxy") {
            addCentralBody();
            generateDisk(count, centralPos, centralVel, config.diskMass, 0u);
            return outParticles.size() >= 2;
        }

        const float galaxySeparation = size * 1.5f;
        const std::uint32_t halfCount = count / 2u;
        const std::uint32_t remainder = count % 2u;
        const float leftMass = requestedParticleMass * static_cast<float>(std::max(1u, halfCount));
        const float rightMass = requestedParticleMass * static_cast<float>(halfCount + remainder);
        const float galaxyMass = std::min(config.diskMass * 0.5f,
                                          std::min(leftMass, rightMass));
        const float orbitalSpeed = std::sqrt(galaxyMass /
                                              std::max(4.0f * galaxySeparation, 0.1f)) * 1.05f;
        generateDisk(halfCount, centralPos + Vector3(-galaxySeparation, 0.0f, 0.0f),
                     centralVel + Vector3(0.0f, orbitalSpeed, 0.0f), config.diskMass * 0.5f, 0u);
        generateDisk(halfCount + remainder,
                     centralPos + Vector3(galaxySeparation, 0.0f, 0.0f),
                     centralVel + Vector3(0.0f, -orbitalSpeed, 0.0f),
                     config.diskMass * 0.5f, 1000u);
        return outParticles.size() >= 2;
    }
    if (mode == "solar_system") {
        const float mercuryR = 0.39f, mercuryM = 1.6e-7f;
        const float venusR = 0.72f, venusM = 2.4e-6f;
        const float earthR = 1.00f, earthM = 3.0e-6f;
        const float marsR = 1.52f, marsM = 3.2e-7f;
        const float jupiterR = 5.20f, jupiterM = 9.5e-4f;
        const float saturnR = 9.54f, saturnM = 2.8e-4f;
        const float uranusR = 19.2f, uranusM = 4.3e-5f;
        const float neptuneR = 30.1f, neptuneM = 5.1e-5f;

        struct Planet {
            float r, m;
        };

        Planet planets[] = {{mercuryR, mercuryM}, {venusR, venusM},     {earthR, earthM},
                            {marsR, marsM},       {jupiterR, jupiterM}, {saturnR, saturnM},
                            {uranusR, uranusM},   {neptuneR, neptuneM}};
        addCentralBody();
        for (const auto& pInfo : planets) {
            std::uniform_real_distribution<float> angleDist(0.0f, kTwoPi);
            const float a = angleDist(rng);
            const float speed =
                std::sqrt(centralMass / pInfo.r) * std::max(0.0f, config.velocityScale);
            Particle p;
            p.setMass(pInfo.m);
            p.setPosition(centralPos + Vector3(pInfo.r * std::cos(a), pInfo.r * std::sin(a), 0.0f));
            p.setVelocity(centralVel + Vector3(-std::sin(a) * speed, std::cos(a) * speed, 0.0f));
            finalizeParticle(p);
            outParticles.push_back(p);
        }
        return outParticles.size() >= 2;
    }
    if (mode == "sph_collapse" || mode == "sph_sphere") {
        const float radius = std::max(0.1f, config.cloudHalfExtent);
        const float mass = std::max(1e-6f, config.particleMass);
        const float radius2 = radius * radius;
        while (outParticles.size() < count) {
            std::uniform_real_distribution<float> dist(-radius, radius);
            const float x = dist(rng);
            const float y = dist(rng);
            const float z = dist(rng);
            if (x * x + y * y + z * z > radius2)
                continue;
            Particle p;
            p.setMass(mass);
            p.setPosition(centralPos + Vector3(x, y, z));
            p.setVelocity(centralVel);
            finalizeParticle(p);
            outParticles.push_back(p);
        }
        return outParticles.size() >= 2;
    }
    // Default generated mode: disk_orbit.
    // Keep orbital initialization consistent with the force model:
    // solvers clamp acceleration magnitude, so cap target orbital
    // acceleration accordingly to avoid injecting super-orbital velocities.
    constexpr float kSolverMaxAcceleration = kPhysicsMaxAccelerationDefault;
    const float radiusMin = std::max(0.01f, std::min(config.diskRadiusMin, config.diskRadiusMax));
    const float radiusMax =
        std::max(radiusMin + 1e-4f, std::max(config.diskRadiusMin, config.diskRadiusMax));
    const float radiusMin2 = radiusMin * radiusMin;
    const float radiusMax2 = radiusMax * radiusMax;
    const float radiusRange2 = std::max(1e-6f, radiusMax2 - radiusMin2);
    const float diskThickness = std::max(0.0f, config.diskThickness);
    const float velocityScale = std::max(0.0f, config.velocityScale);
    const float effectiveCentralMass = config.includeCentralBody ? centralMass : 0.0f;
    const float effectiveDiskMass = std::max(0.0f, config.diskMass);
    std::uniform_real_distribution<float> radiusDist(radiusMin, radiusMax);
    std::uniform_real_distribution<float> angleDist(0.0f, kTwoPi);
    std::uniform_real_distribution<float> zDist(-diskThickness, diskThickness);
    addCentralBody();
    const std::uint32_t diskCount =
        std::max<std::uint32_t>(1u, count - static_cast<std::uint32_t>(outParticles.size()));
    const float diskMassPerParticle =
        std::max(1e-6f, config.diskMass / static_cast<float>(diskCount));

    // Pre-generate random orbital parameters sequentially for determinism
    RandomData orbitalData;
    orbitalData.reserve(diskCount, 3);  // 3 floats per particle: r, angle, z
    for (std::uint32_t i = 0; i < diskCount; ++i) {
        orbitalData.push(radiusDist(rng));
        orbitalData.push(angleDist(rng));
        orbitalData.push(zDist(rng));
    }

    // Parallel particle fabrication from pre-generated orbital parameters
    outParticles.reserve(outParticles.size() + diskCount);
#pragma omp parallel if (!config.deterministicMode)
    {
        std::vector<Particle> threadParticles;
        const std::ptrdiff_t particleTotal = static_cast<std::ptrdiff_t>(orbitalData.particleCount());
#pragma omp for nowait
        for (std::ptrdiff_t i = 0; i < particleTotal; ++i) {
            const std::size_t particleIndex = static_cast<std::size_t>(i);
            const float r = orbitalData.get(particleIndex, 0);
            const float angle = orbitalData.get(particleIndex, 1);
            const float z = orbitalData.get(particleIndex, 2);
            const Vector3 radial(r * std::cos(angle), r * std::sin(angle), z);
            const Vector3 position = centralPos + radial;
            const float enclosedFraction = std::clamp((r * r - radiusMin2) / radiusRange2, 0.0f, 1.0f);
            const float enclosedMass =
                std::max(1e-6f, effectiveCentralMass + effectiveDiskMass * enclosedFraction);
            const float blitzarAccel = enclosedMass / std::max(r * r, 1e-6f);
            const float cappedAccel = std::min(blitzarAccel, kSolverMaxAcceleration);
            const float orbitalSpeed = std::sqrt(cappedAccel * std::max(r, 1e-4f)) * velocityScale;
            Vector3 tangent(-std::sin(angle), std::cos(angle), 0.0f);
            tangent = tangent * orbitalSpeed;
            Particle p;
            p.setMass(diskMassPerParticle);
            p.setPosition(position);
            p.setVelocity(centralVel + tangent);
            finalizeParticle(p);
            threadParticles.push_back(p);
        }
#pragma omp critical
        outParticles.insert(outParticles.end(), threadParticles.begin(), threadParticles.end());
    }
    return outParticles.size() >= 2;
}

static std::string sceneObjectMode(const SceneObjectConfig& object)
{
    const std::string type = toLower(object.type);
    if (type == "circle") return "disk_orbit";
    if (type == "cloud") return "random_cloud";
    if (type == "plummer" || type == "plummer_sphere") return "plummer_sphere";
    if (type == "galaxy") return "galaxy";
    if (type == "galaxy_collision") return "galaxy_collision";
    if (type == "binary") return "two_body";
    if (type == "binary_star") return "two_body";
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
    const bool pivotProperty = hasSceneObjectProperty(object, "pivot");
    child.sceneOffsetX = offset ? object.offsetX : 0.0f;
    child.sceneOffsetY = offset ? object.offsetY : 0.0f;
    child.sceneOffsetZ = offset ? object.offsetZ : 0.0f;
    const std::string pivot = pivotProperty ? toLower(object.pivot) : "world";
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
    if (id.empty())
        return nullptr;
    for (const SceneObjectConfig& object : scene.objects) {
        if (object.id == id)
            return &object;
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
    if (emitter == nullptr || instance == nullptr || property.particleCount == 0u)
        return false;

    std::vector<Particle> emitterParticles;
    if (!buildSceneObjectBase(emitterParticles, *emitter, config) || emitterParticles.empty())
        return false;
    particles.reserve(particles.size() +
                      static_cast<std::size_t>(property.particleCount) *
                          static_cast<std::size_t>(std::max<std::uint32_t>(2u,
                                                                            instance->particleCount)));
    for (std::uint32_t index = 0u; index < property.particleCount; ++index) {
        const std::uint64_t scaledIndex =
            static_cast<std::uint64_t>(index) * emitterParticles.size();
        const std::size_t emitterIndex =
            static_cast<std::size_t>((scaledIndex / property.particleCount) %
                                     emitterParticles.size());
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
        if (!buildSceneObjectBase(generated, instanceObject, config))
            continue;
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
    if (count == 0u)
        return;
    if (appendReferencedObjectSystem(particles, property, scene, config))
        return;
    if (particles.empty())
        return;

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
            1u, std::min(count, static_cast<std::uint32_t>(std::ceil(std::sqrt(
                static_cast<float>(count) / 24.0f)))));
        treeCenters.reserve(treeCount);
        for (std::uint32_t tree = 0u; tree < treeCount; ++tree)
            treeCenters.push_back(randomDiskPoint(rng, size));
    }
    particles.reserve(particles.size() + static_cast<std::size_t>(count));
    for (std::uint32_t index = 0u; index < count; ++index) {
        Vector3 local(0.0f, 0.0f, 0.0f);
        if (distribution == "forest") {
            const Vector3 center = treeCenters[index % treeCenters.size()];
            local = center + Vector3(unit(rng) * size * 0.08f,
                                     unit(rng) * size * 0.08f,
                                     positive(rng) * height);
        }
        else if (distribution == "uniform_box") {
            local = Vector3(unit(rng) * size, unit(rng) * size, unit(rng) * size);
        }
        else {
            for (std::uint32_t attempt = 0u; attempt < 16u; ++attempt) {
                local = Vector3(unit(rng) * size, unit(rng) * size, unit(rng) * size);
                if (local.x * local.x + local.y * local.y + local.z * local.z <= size * size)
                    break;
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
        if (!object.enabled)
            continue;
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
                (void)applyInitialStateTransform(source,
                                                 makeObjectInitialState(config, object));
                outParticles.insert(outParticles.end(), source.begin(), source.end());
            }
            continue;
        }
        std::vector<Particle> objectParticles;
        if (!buildSceneObjectBase(objectParticles, object, config))
            continue;
        const std::size_t baseCount = objectParticles.size();
        if (hasSceneObjectProperty(object, "particle_system")) {
            appendParticleSystem(objectParticles, object, config.scene, config);
            if (objectParticles.size() > baseCount) {
                std::vector<Particle> generated(objectParticles.begin() +
                                                    static_cast<std::ptrdiff_t>(baseCount),
                                                objectParticles.end());
                (void)applyInitialStateTransform(generated,
                                                 makeObjectInitialState(config, object));
                outParticles.insert(outParticles.end(), generated.begin(), generated.end());
                objectParticles.resize(baseCount);
            }
        }
        (void)applyInitialStateTransform(objectParticles,
                                         makeObjectInitialState(config, object));
        outParticles.insert(outParticles.end(), objectParticles.begin(), objectParticles.end());
    }
    return outParticles.size() >= 2u;
}

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
    std::vector<Particle> source = particles;
    std::vector<Particle> transformed;
    transformed.reserve(outputCount);
    const bool mirrorX = config.sceneMirrorX;
    const bool mirrorY = config.sceneMirrorY;
    const bool mirrorZ = config.sceneMirrorZ;
    const Vector3 pivot(config.scenePivotX, config.scenePivotY, config.scenePivotZ);
    for (std::uint32_t mirrorMask = 0u; mirrorMask < mirrorVariants; ++mirrorMask) {
        const bool flipX = mirrorX && ((mirrorMask & xBit) != 0u);
        const bool flipY = mirrorY && ((mirrorMask & yBit) != 0u);
        const bool flipZ = mirrorZ && ((mirrorMask & zBit) != 0u);
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

/*
 * @brief Documents the atomic add float operation contract.
 * @param atom Input value used by this contract.
 * @param val Input value used by this contract.
 * @return No return value.
 * @note Keep side effects explicit and preserve deterministic behavior where callers depend on it.
 */
void atomicAddFloat(std::atomic<float>& atom, float val)
{
    float current = atom.load(std::memory_order_relaxed);
    while (!atom.compare_exchange_weak(current, current + val, std::memory_order_relaxed)) {
        // current is updated with the latest value on failure
    }
}
