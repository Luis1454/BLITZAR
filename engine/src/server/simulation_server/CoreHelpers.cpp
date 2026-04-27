// File: engine/src/server/simulation_server/CoreHelpers.cpp
// Purpose: Engine implementation for the BLITZAR simulation core.

#include "Internal.hpp"

/// Description: Executes the toLower operation.
std::string toLower(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

/// Description: Executes the trim operation.
std::string trim(std::string value)
{
    const auto begin = std::find_if_not(value.begin(), value.end(), [](unsigned char c) {
        return std::isspace(c) != 0;
    });
    const auto end = std::find_if_not(value.rbegin(), value.rend(), [](unsigned char c) {
                         return std::isspace(c) != 0;
                     }).base();
    if (begin >= end)
        return {};
    return std::string(begin, end);
}

/// Description: Executes the normalizeSnapshotFormat operation.
std::string normalizeSnapshotFormat(std::string format)
{
    format = toLower(std::move(format));
    static const std::array<std::pair<std::string_view, std::string_view>, 4> aliases = {
        {{"binary", "bin"},
         {"vtkb", "vtk_binary"},
         {"vtk-bin", "vtk_binary"},
         {"vtk_ascii", "vtk"}}};
    for (const auto& entry : aliases) {
        if (format == entry.first) {
            return std::string(entry.second);
        }
    }
    return format;
}

/// Description: Executes the solverModeFromCanonicalName operation.
ParticleSystem::SolverMode solverModeFromCanonicalName(std::string_view name)
{
    static const std::array<std::pair<std::string_view, ParticleSystem::SolverMode>, 2> modes = {
        {{grav_modes::kSolverOctreeCpu, ParticleSystem::SolverMode::OctreeCpu},
         {grav_modes::kSolverOctreeGpu, ParticleSystem::SolverMode::OctreeGpu}}};
    for (const auto& entry : modes) {
        if (name == entry.first) {
            return entry.second;
        }
    }
    return ParticleSystem::SolverMode::PairwiseCuda;
}

/// Description: Executes the integratorModeFromCanonicalName operation.
ParticleSystem::IntegratorMode integratorModeFromCanonicalName(std::string_view name)
{
    static const std::array<std::pair<std::string_view, ParticleSystem::IntegratorMode>, 1> modes =
        {{{grav_modes::kIntegratorRk4, ParticleSystem::IntegratorMode::Rk4}}};
    for (const auto& entry : modes) {
        if (name == entry.first) {
            return entry.second;
        }
    }
    return ParticleSystem::IntegratorMode::Euler;
}

/// Description: Executes the solverLabel operation.
std::string_view solverLabel(ParticleSystem::SolverMode mode)
{
    switch (mode) {
    case ParticleSystem::SolverMode::OctreeCpu:
        return "octree_cpu";
    case ParticleSystem::SolverMode::OctreeGpu:
        return "octree_gpu";
    case ParticleSystem::SolverMode::PairwiseCuda:
    default:
        return "pairwise_cuda";
    }
}

/// Description: Executes the resolvePublishedSnapshotCap operation.
std::uint32_t resolvePublishedSnapshotCap(std::uint32_t drawCap)
{
    const std::uint32_t clampedDrawCap =
        grav_protocol::clampSnapshotPoints(std::max(grav_protocol::kSnapshotMinPoints, drawCap));
    const std::uint32_t oversampled =
        std::min<std::uint32_t>(grav_protocol::kSnapshotMaxPoints,
                                std::max<std::uint32_t>(clampedDrawCap, clampedDrawCap * 2u));
    return std::max(grav_protocol::kSnapshotMinPoints, oversampled);
}

/// Description: Executes the readRawBytes operation.
bool readRawBytes(std::istream& in, std::byte* data, std::size_t size)
{
    return static_cast<bool>(
        in.read(reinterpret_cast<char*>(data), static_cast<std::streamsize>(size)));
}

/// Description: Executes the writeRawBytes operation.
bool writeRawBytes(std::ostream& out, const std::byte* data, std::size_t size)
{
    out.write(reinterpret_cast<const char*>(data), static_cast<std::streamsize>(size));
    return static_cast<bool>(out);
}

/// Description: Executes the readEnvironment operation.
std::string readEnvironment(std::string_view key)
{
    const std::optional<std::string> value = grav_env::get(key);
    return value.value_or(std::string{});
}

/// Description: Executes the isValidImportedParticleCount operation.
bool isValidImportedParticleCount(std::size_t count)
{
    return count >= 2 && count <= kMaxImportedParticles;
}

/// Description: Executes the isAutoSolverFallbackEnabled operation.
bool isAutoSolverFallbackEnabled()
{
    const std::string raw = readEnvironment("GRAVITY_AUTO_SOLVER_FALLBACK");
    if (raw.empty()) {
        return false;
    }
    const std::string v = toLower(trim(raw));
    return v == "1" || v == "true" || v == "on" || v == "yes";
}

/// Description: Executes the shouldForceCudaFailureOnceForTesting operation.
bool shouldForceCudaFailureOnceForTesting(std::string_view solver)
{
    if (solver != grav_modes::kSolverPairwiseCuda && solver != grav_modes::kSolverOctreeGpu)
        return false;
    const std::string raw = readEnvironment("GRAVITY_TEST_FORCE_CUDA_FAIL_ONCE");
    if (raw.empty()) {
        return false;
    }
    const std::string v = toLower(trim(raw));
    if (!(v == "1" || v == "true" || v == "on" || v == "yes")) {
        return false;
    }
    static std::atomic<bool> injected{false};
    bool expected = false;
    return injected.compare_exchange_strong(expected, true, std::memory_order_relaxed);
}

/// Description: Describes the coerce config solver integrator compatibility operation contract.
bool coerceConfigSolverIntegratorCompatibility(std::string& solver, std::string& integrator,
                                               std::string_view source)
{
    if (!grav_modes::isSupportedSolverIntegratorPair(solver, integrator)) {
        std::cerr << "[server] " << source
                  << ": solver octree_gpu requires integrator euler, using euler\n";
        integrator.assign(grav_modes::kIntegratorEuler);
        return true;
    }
    return false;
}

/// Description: Describes the auto target substep dt operation contract.
float autoTargetSubstepDt(std::string_view solver, bool eulerIntegrator, bool sphEnabled,
                          std::size_t liveParticleCount)
{
    if (sphEnabled)
        return 0.001f;
    if (!eulerIntegrator)
        return 0.0025f;
    if (liveParticleCount >= 100'000u)
        return 0.01f;
    if (liveParticleCount >= 20'000u)
        return 0.005f;
    if (solver == grav_modes::kSolverOctreeGpu)
        return 0.0025f;
    if (solver == grav_modes::kSolverOctreeCpu)
        return 0.001f;
    return 0.0005f;
}

/// Description: Executes the openingCriterionFromCanonicalName operation.
OctreeOpeningCriterion openingCriterionFromCanonicalName(std::string_view name)
{
    static const std::array<std::pair<std::string_view, OctreeOpeningCriterion>, 1> criteria = {
        {{grav_modes::kOctreeCriterionBounds, OctreeOpeningCriterion::Bounds}}};
    for (const auto& entry : criteria) {
        if (name == entry.first) {
            return entry.second;
        }
    }
    return OctreeOpeningCriterion::CenterOfMass;
}

/// Description: Executes the clampThetaBound operation.
float clampThetaBound(float value)
{
    return std::clamp(value, 0.05f, 4.0f);
}

/// Description: Executes the computeOctreeDistributionScore operation.
float computeOctreeDistributionScore(const std::vector<Particle>& particles)
{
    if (particles.empty()) {
        return 0.0f;
    }
    Vector3 minPos = particles.front().getPosition();
    Vector3 maxPos = minPos;
    for (const Particle& particle : particles) {
        const Vector3 pos = particle.getPosition();
        minPos.x = std::min(minPos.x, pos.x);
        minPos.y = std::min(minPos.y, pos.y);
        minPos.z = std::min(minPos.z, pos.z);
        maxPos.x = std::max(maxPos.x, pos.x);
        maxPos.y = std::max(maxPos.y, pos.y);
        maxPos.z = std::max(maxPos.z, pos.z);
    }
    const Vector3 center(0.5f * (minPos.x + maxPos.x), 0.5f * (minPos.y + maxPos.y),
                         0.5f * (minPos.z + maxPos.z));
    std::array<std::size_t, 8> octants{};
    for (const Particle& particle : particles) {
        const Vector3 pos = particle.getPosition();
        int index = 0;
        if (pos.x >= center.x) {
            index |= 1;
        }
        if (pos.y >= center.y) {
            index |= 2;
        }
        if (pos.z >= center.z) {
            index |= 4;
        }
        octants[static_cast<std::size_t>(index)] += 1u;
    }
    std::size_t activeOctants = 0u;
    std::size_t dominantCount = 0u;
    for (const std::size_t count : octants) {
        if (count > 0u) {
            activeOctants += 1u;
            dominantCount = std::max(dominantCount, count);
        }
    }
    const float occupancyScore = static_cast<float>(activeOctants) / 8.0f;
    const float dominantFraction =
        static_cast<float>(dominantCount) / static_cast<float>(particles.size());
    return std::clamp(0.6f * occupancyScore + 0.4f * (1.0f - dominantFraction), 0.0f, 1.0f);
}
