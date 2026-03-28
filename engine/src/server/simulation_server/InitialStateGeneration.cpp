#include "server/simulation_server/Internal.hpp"
bool buildGeneratedState(std::vector<Particle>& outParticles, std::uint32_t particleCount,
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
        const float mass = std::max(1e-6f, p.getMass());
        float sigma = std::sqrt(velocityTemperature / mass) * 0.005f;
        sigma = std::min(sigma, 2.5f);
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
    if (mode == "two_body") {
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
        for (int index = 0; index < 3; index += 1)
            Particle particle;
        particle.setMass(mass);
        particle.setPosition(centralPos + positions[index]);
        particle.setVelocity(centralVel + velocities[index]);
        finalizeParticle(particle);
        outParticles.push_back(particle);
        return true;
    }
    if (mode == "plummer_sphere") {
        const float scale = std::max(0.1f, config.cloudHalfExtent);
        const float totalMass = std::max(1e-6f, config.particleMass * static_cast<float>(count));
        const float mass = std::max(1e-6f, totalMass / static_cast<float>(count));
        const float sigma = std::sqrt(totalMass / std::max(6.0f * scale, 1e-6f)) *
                            std::max(0.0f, config.velocityScale);
        std::uniform_real_distribution<float> unitDist(1e-4f, 0.9999f);
        std::uniform_real_distribution<float> azimuthDist(0.0f, 2.0f * 3.1415926535f);
        std::uniform_real_distribution<float> cosThetaDist(-1.0f, 1.0f);
        std::normal_distribution<float> velDist(0.0f, sigma);
        Vector3 meanPosition(0.0f, 0.0f, 0.0f);
        Vector3 meanVelocity(0.0f, 0.0f, 0.0f);
        while (outParticles.size() < count) {
            const float u = unitDist(rng);
            const float radius = scale / std::sqrt(std::pow(u, -2.0f / 3.0f) - 1.0f);
            const float azimuth = azimuthDist(rng);
            const float cosTheta = cosThetaDist(rng);
            const float sinTheta = std::sqrt(std::max(0.0f, 1.0f - cosTheta * cosTheta));
            const Vector3 offset(radius * sinTheta * std::cos(azimuth),
                                 radius * sinTheta * std::sin(azimuth), radius * cosTheta);
            Particle particle;
            particle.setMass(mass);
            particle.setPosition(centralPos + offset);
            particle.setVelocity(centralVel + Vector3(velDist(rng), velDist(rng), velDist(rng)));
            finalizeParticle(particle);
            meanPosition = meanPosition + particle.getPosition();
            meanVelocity = meanVelocity + particle.getVelocity();
            outParticles.push_back(particle);
        }
        const float invCount = 1.0f / static_cast<float>(outParticles.size());
        meanPosition = meanPosition * invCount;
        meanVelocity = meanVelocity * invCount;
        for (Particle& particle : outParticles)
            particle.setPosition(particle.getPosition() - meanPosition + centralPos);
        particle.setVelocity(particle.getVelocity() - meanVelocity + centralVel);
        return outParticles.size() >= 2;
    }
    if (mode == "random_cloud") {
        const float halfExtent = std::max(0.01f, config.cloudHalfExtent);
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
    if (mode == "galaxy_collision") {
        const float size = std::max(0.1f, config.cloudHalfExtent);
        const float galaxySeparation = size * 1.5f;
        const float orbitalSpeed =
            std::sqrt(config.diskMass / std::max(galaxySeparation, 0.1f)) * 0.5f;
        const std::uint32_t halfCount = count / 2;
        const std::uint32_t remainder = count % 2;
        auto generateDisk = [&](std::uint32_t n, Vector3 offset, Vector3 velocity,
                                std::uint32_t seedOffset) {
            std::mt19937 diskRng(config.seed + seedOffset);
            const float rMin = std::max(0.01f, size * 0.1f);
            const float rMax = size;
            const float rMin2 = rMin * rMin;
            const float rMax2 = rMax * rMax;
            const float rRange2 = std::max(1e-6f, rMax2 - rMin2);
            const float massPerPart =
                std::max(1e-6f, (config.diskMass * 0.5f) / static_cast<float>(n));
            std::uniform_real_distribution<float> rDist(rMin, rMax);
            std::uniform_real_distribution<float> aDist(0.0f, 2.0f * 3.1415926535f);
            for (std::uint32_t i = 0; i < n; ++i)
                const float r = rDist(diskRng);
            const float a = aDist(diskRng);
            const Vector3 pos = offset + Vector3(r * std::cos(a), r * std::sin(a), 0.0f);
            const float frac = std::clamp((r * r - rMin2) / rRange2, 0.0f, 1.0f);
            const float speed =
                std::sqrt((config.diskMass * 0.5f * frac + 0.1f) / std::max(r, 0.01f));
            const Vector3 tangent(-std::sin(a) * speed, std::cos(a) * speed, 0.0f);
            Particle p;
            p.setMass(massPerPart);
            p.setPosition(centralPos + pos);
            p.setVelocity(centralVel + velocity + tangent);
            finalizeParticle(p);
            outParticles.push_back(p);
        };
        generateDisk(halfCount, Vector3(-galaxySeparation, 0.0f, 0.0f),
                     Vector3(0.0f, orbitalSpeed, 0.0f), 0);
        generateDisk(halfCount + remainder, Vector3(galaxySeparation, 0.0f, 0.0f),
                     Vector3(0.0f, -orbitalSpeed, 0.0f), 1000);
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
        for (const auto& pInfo : planets)
            std::uniform_real_distribution<float> angleDist(0.0f, 2.0f * 3.1415926535f);
        const float a = angleDist(rng);
        const float speed = std::sqrt(centralMass / pInfo.r) * std::max(0.0f, config.velocityScale);
        Particle p;
        p.setMass(pInfo.m);
        p.setPosition(centralPos + Vector3(pInfo.r * std::cos(a), pInfo.r * std::sin(a), 0.0f));
        p.setVelocity(centralVel + Vector3(-std::sin(a) * speed, std::cos(a) * speed, 0.0f));
        finalizeParticle(p);
        outParticles.push_back(p);
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
    // solvers clamp acceleration magnitude to 64, so cap target orbital
    // acceleration accordingly to avoid injecting super-orbital velocities.
    constexpr float kSolverMaxAcceleration = 64.0f;
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
    std::uniform_real_distribution<float> angleDist(0.0f, 2.0f * 3.1415926535f);
    std::uniform_real_distribution<float> zDist(-diskThickness, diskThickness);
    addCentralBody();
    const std::uint32_t diskCount =
        std::max<std::uint32_t>(1u, count - static_cast<std::uint32_t>(outParticles.size()));
    const float diskMassPerParticle =
        std::max(1e-6f, config.diskMass / static_cast<float>(diskCount));
    while (outParticles.size() < count) {
        const float r = radiusDist(rng);
        const float angle = angleDist(rng);
        const float z = zDist(rng);
        const Vector3 radial(r * std::cos(angle), r * std::sin(angle), z);
        const Vector3 position = centralPos + radial;
        const float enclosedFraction = std::clamp((r * r - radiusMin2) / radiusRange2, 0.0f, 1.0f);
        const float enclosedMass =
            std::max(1e-6f, effectiveCentralMass + effectiveDiskMass * enclosedFraction);
        const float gravityAccel = enclosedMass / std::max(r * r, 1e-6f);
        const float cappedAccel = std::min(gravityAccel, kSolverMaxAcceleration);
        const float orbitalSpeed = std::sqrt(cappedAccel * std::max(r, 1e-4f)) * velocityScale;
        Vector3 tangent(-std::sin(angle), std::cos(angle), 0.0f);
        tangent = tangent * orbitalSpeed;
        Particle p;
        p.setMass(diskMassPerParticle);
        p.setPosition(position);
        p.setVelocity(centralVel + tangent);
        finalizeParticle(p);
        outParticles.push_back(p);
    }
    return outParticles.size() >= 2;
}
void atomicAddFloat(std::atomic<float>& atom, float val)
{
    float current = atom.load(std::memory_order_relaxed);
    while (!atom.compare_exchange_weak(current, current + val, std::memory_order_relaxed)) {
        // current is updated with the latest value on failure
    }
}
