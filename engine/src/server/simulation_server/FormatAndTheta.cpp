#include "server/simulation_server/Internal.hpp"
float profileThetaBias(std::string_view performanceProfile)
{
    if (performanceProfile == "interactive")
        return 0.9f;
    if (performanceProfile == "balanced")
        return 0.6f;
    if (performanceProfile == "quality")
        return 0.2f;
    return 0.5f;
}
float particleThetaBias(std::size_t particleCount)
{
    if (particleCount <= 512u)
        return 0.0f;
    const float normalized =
        static_cast<float>(particleCount - 512u) / static_cast<float>(65536u - 512u);
    return std::clamp(normalized, 0.0f, 1.0f);
}
float resolveOctreeTheta(float configuredTheta, bool autoTune, float autoMin, float autoMax,
                         std::string_view performanceProfile,
                         const std::vector<Particle>& particles, float distributionScore)
{
    const float clampedMin = clampThetaBound(autoMin);
    const float clampedMax = std::max(clampedMin, clampThetaBound(autoMax));
    const float clampedConfigured = std::clamp(configuredTheta, clampedMin, clampedMax);
    if (!autoTune)
        return clampedConfigured;
    const float span = std::max(1e-6f, clampedMax - clampedMin);
    const float configuredBias = std::clamp((clampedConfigured - clampedMin) / span, 0.0f, 1.0f);
    const float blendedBias = std::clamp(0.45f * profileThetaBias(performanceProfile) +
                                             0.25f * particleThetaBias(particles.size()) +
                                             0.15f * distributionScore + 0.15f * configuredBias,
                                         0.0f, 1.0f);
    return clampedMin + span * blendedBias;
}
void logEffectiveExecutionModes(
    std::string_view solver, std::string_view integrator, std::string_view performanceProfile,
    std::string_view openingCriterion, float theta, float effectiveTheta, bool thetaAutoTune,
    float thetaAutoMin, float thetaAutoMax, float octreeDistributionScore, float softening,
    float physicsMaxAcceleration, float physicsMinSoftening, float physicsMinDistance2,
    float physicsMinTheta, bool sphEnabled, float configuredSubstepTargetDt,
    std::uint32_t configuredMaxSubsteps, std::uint32_t snapshotPublishPeriodMs, float serverFps,
    float energyDriftPct)
{
    std::cout << "[server] active solver=" << solver << " integrator=" << integrator
              << " perf=" << performanceProfile;
    if (solver == grav_modes::kSolverOctreeCpu || solver == grav_modes::kSolverOctreeGpu) {
        std::cout << " criterion=" << openingCriterion << " theta=" << theta
                  << " theta_effective=" << effectiveTheta
                  << " theta_auto=" << (thetaAutoTune ? "on" : "off")
                  << " theta_auto_min=" << thetaAutoMin << " theta_auto_max=" << thetaAutoMax
                  << " distribution_score=" << octreeDistributionScore
                  << " softening=" << softening;
    }
    std::cout << " physics_max_acceleration=" << physicsMaxAcceleration
              << " physics_min_softening=" << physicsMinSoftening
              << " physics_min_distance2=" << physicsMinDistance2
              << " physics_min_theta=" << physicsMinTheta;
    std::cout << " sph=" << (sphEnabled ? "on" : "off")
              << " substep_target_dt=" << configuredSubstepTargetDt
              << " max_substeps=" << configuredMaxSubsteps
              << " snapshot_publish_ms=" << snapshotPublishPeriodMs << " perf_fps=" << serverFps
              << " error_energy_drift_pct=" << energyDriftPct << "\n";
}
std::string defaultExportPath(const std::string& directory, const std::string& format,
                              std::uint64_t step)
{
    const auto now = std::chrono::system_clock::now();
    const std::time_t nowTime = std::chrono::system_clock::to_time_t(now);
    const std::tm tm = grav_platform::localTime(nowTime);
    std::string extension = normalizeSnapshotFormat(format);
    if (extension == "vtk_binary")
        extension = "vtk";
    std::ostringstream fileName;
    fileName << "sim_" << std::put_time(&tm, "%Y%m%d_%H%M%S") << "_s" << step << "." << extension;
    std::filesystem::path outPath = std::filesystem::path(directory) / fileName.str();
    return outPath.string();
}
std::string guessFormatFromPath(const std::string& path)
{
    const std::filesystem::path p(path);
    std::string ext = toLower(p.extension().string());
    if (!ext.empty() && ext[0] == '.') {
        ext.erase(ext.begin());
    }
    if (ext == "binary" || ext == "nbin")
        return "bin";
    if (ext == "vtkb")
        return "vtk_binary";
    return ext;
}
bool readBeU32(std::istream& in, std::uint32_t& outValue)
{
    std::array<std::byte, 4> bytes{std::byte{0}, std::byte{0}, std::byte{0}, std::byte{0}};
    if (!readRawBytes(in, bytes.data(), bytes.size())) {
        return false;
    }
    outValue = (static_cast<std::uint32_t>(std::to_integer<unsigned char>(bytes[0])) << 24) |
               (static_cast<std::uint32_t>(std::to_integer<unsigned char>(bytes[1])) << 16) |
               (static_cast<std::uint32_t>(std::to_integer<unsigned char>(bytes[2])) << 8) |
               static_cast<std::uint32_t>(std::to_integer<unsigned char>(bytes[3]));
    return true;
}
bool readBeI32(std::istream& in, std::int32_t& outValue)
{
    std::uint32_t unsignedValue = 0u;
    if (!readBeU32(in, unsignedValue)) {
        return false;
    }
    outValue = static_cast<std::int32_t>(unsignedValue);
    return true;
}
bool readBeF32(std::istream& in, float& outValue)
{
    std::uint32_t bits = 0u;
    if (!readBeU32(in, bits)) {
        return false;
    }
    std::memcpy(&outValue, &bits, sizeof(float));
    return true;
}
void writeBeU32(std::ostream& out, std::uint32_t value)
{
    const std::array<std::byte, 4> bytes{
        std::byte{static_cast<unsigned char>((value >> 24) & 0xFFu)},
        std::byte{static_cast<unsigned char>((value >> 16) & 0xFFu)},
        std::byte{static_cast<unsigned char>((value >> 8) & 0xFFu)},
        std::byte{static_cast<unsigned char>(value & 0xFFu)}};
    (void)writeRawBytes(out, bytes.data(), bytes.size());
}
void writeBeI32(std::ostream& out, std::int32_t value)
{
    writeBeU32(out, static_cast<std::uint32_t>(value));
}
void writeBeF32(std::ostream& out, float value)
{
    std::uint32_t bits = 0u;
    std::memcpy(&bits, &value, sizeof(float));
    writeBeU32(out, bits);
}
