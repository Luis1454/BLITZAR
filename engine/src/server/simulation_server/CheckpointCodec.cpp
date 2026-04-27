// File: engine/src/server/simulation_server/CheckpointCodec.cpp
// Purpose: Engine implementation for the BLITZAR simulation core.

#include "Internal.hpp"
/// Description: Executes the consumeOptionalLineBreak operation.
void consumeOptionalLineBreak(std::istream& in)
{
    const int next = in.peek();
    if (next == '\n') {
        in.get();
        return;
    }
    if (next == '\r') {
        in.get();
        if (in.peek() == '\n') {
            in.get();
        }
    }
}
/// Description: Executes the readLeU64 operation.
bool readLeU64(std::istream& in, std::uint64_t& outValue)
{
    std::array<std::byte, 8> bytes{};
    if (!readRawBytes(in, bytes.data(), bytes.size())) {
        return false;
    }
    outValue = 0u;
    for (std::size_t index = 0; index < bytes.size(); ++index) {
        outValue |= static_cast<std::uint64_t>(std::to_integer<unsigned char>(bytes[index]))
                    << (index * 8u);
    }
    return true;
}
/// Description: Executes the writeLeU64 operation.
void writeLeU64(std::ostream& out, std::uint64_t value)
{
    std::array<std::byte, 8> bytes{};
    for (std::size_t index = 0; index < bytes.size(); ++index) {
        bytes[index] = std::byte{static_cast<unsigned char>((value >> (index * 8u)) & 0xFFu)};
    }
    (void)writeRawBytes(out, bytes.data(), bytes.size());
}
/// Description: Executes the writeLeU32 operation.
void writeLeU32(std::ostream& out, std::uint32_t value)
{
    std::array<std::byte, 4> bytes{};
    for (std::size_t index = 0; index < bytes.size(); ++index) {
        bytes[index] = std::byte{static_cast<unsigned char>((value >> (index * 8u)) & 0xFFu)};
    }
    (void)writeRawBytes(out, bytes.data(), bytes.size());
}
/// Description: Executes the readLeU32 operation.
bool readLeU32(std::istream& in, std::uint32_t& outValue)
{
    std::array<std::byte, 4> bytes{};
    if (!readRawBytes(in, bytes.data(), bytes.size())) {
        return false;
    }
    outValue = 0u;
    for (std::size_t index = 0; index < bytes.size(); ++index) {
        outValue |= static_cast<std::uint32_t>(std::to_integer<unsigned char>(bytes[index]))
                    << (index * 8u);
    }
    return true;
}
/// Description: Executes the writeLeF32 operation.
void writeLeF32(std::ostream& out, float value)
{
    std::uint32_t bits = 0u;
    /// Description: Executes the memcpy operation.
    std::memcpy(&bits, &value, sizeof(bits));
    /// Description: Executes the writeLeU32 operation.
    writeLeU32(out, bits);
}
/// Description: Executes the readLeF32 operation.
bool readLeF32(std::istream& in, float& outValue)
{
    std::uint32_t bits = 0u;
    if (!readLeU32(in, bits)) {
        return false;
    }
    /// Description: Executes the memcpy operation.
    std::memcpy(&outValue, &bits, sizeof(outValue));
    return true;
}
/// Description: Executes the writeSizedString operation.
bool writeSizedString(std::ostream& out, const std::string& value)
{
    if (value.size() > static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max())) {
        return false;
    }
    /// Description: Executes the writeLeU32 operation.
    writeLeU32(out, static_cast<std::uint32_t>(value.size()));
    if (value.empty()) {
        return static_cast<bool>(out);
    }
    out.write(value.data(), static_cast<std::streamsize>(value.size()));
    return static_cast<bool>(out);
}
/// Description: Executes the readSizedString operation.
bool readSizedString(std::istream& in, std::string& outValue, std::size_t maxLength)
{
    std::uint32_t size = 0u;
    if (!readLeU32(in, size) || size > maxLength) {
        return false;
    }
    outValue.assign(static_cast<std::size_t>(size), '\0');
    if (size == 0u)
        return true;
    return static_cast<bool>(in.read(outValue.data(), static_cast<std::streamsize>(size)));
}
bool isSupportedCheckpointString(std::string_view solver, std::string_view integrator,
                                 std::string_view profile, std::string_view criterion)
{
    std::string normalizedSolver;
    std::string normalizedIntegrator;
    std::string normalizedProfile;
    std::string normalizedCriterion;
    return grav_modes::normalizeSolver(std::string(solver), normalizedSolver) &&
           grav_modes::normalizeIntegrator(std::string(integrator), normalizedIntegrator) &&
           grav_config::normalizePerformanceProfile(std::string(profile), normalizedProfile) &&
           /// Description: Executes the normalizeOctreeOpeningCriterion operation.
           grav_modes::normalizeOctreeOpeningCriterion(std::string(criterion), normalizedCriterion);
}
bool writeCheckpointFile(const std::string& outputPath, const SimulationCheckpointState& state,
                         std::string* outError)
{
    /// Description: Executes the outPath operation.
    std::filesystem::path outPath(outputPath);
    if (outPath.has_parent_path()) {
        std::error_code ec;
        /// Description: Executes the create_directories operation.
        std::filesystem::create_directories(outPath.parent_path(), ec);
    }
    /// Description: Executes the out operation.
    std::ofstream out(outputPath, std::ios::binary | std::ios::trunc);
    if (!out.is_open()) {
        if (outError != nullptr) {
            *outError = "could not open checkpoint output";
        }
        return false;
    }
    if (!isValidImportedParticleCount(state.particles.size()) ||
        !isSupportedCheckpointString(state.config.solver, state.config.integrator,
                                     state.config.performanceProfile,
                                     state.config.octreeOpeningCriterion)) {
        if (outError != nullptr) {
            *outError = "checkpoint state is not serializable";
        }
        return false;
    }
    const std::uint32_t flags =
        (state.paused ? kCheckpointFlagPaused : 0u) |
        (state.hasEnergyBaseline ? kCheckpointFlagHasEnergyBaseline : 0u) |
        (state.config.sphEnabled ? kCheckpointFlagSphEnabled : 0u) |
        (state.config.octreeThetaAutoTune ? kCheckpointFlagThetaAutoTune : 0u) |
        (state.gpuTelemetryEnabled ? kCheckpointFlagGpuTelemetryEnabled : 0u);
    if (!writeRawBytes(out, reinterpret_cast<const std::byte*>(kCheckpointMagic),
                       sizeof(kCheckpointMagic))) {
        if (outError != nullptr) {
            *outError = "could not write checkpoint header";
        }
        return false;
    }
    /// Description: Executes the writeLeU32 operation.
    writeLeU32(out, kCheckpointVersion);
    /// Description: Executes the writeLeU32 operation.
    writeLeU32(out, flags);
    /// Description: Executes the writeLeU64 operation.
    writeLeU64(out, state.steps);
    /// Description: Executes the writeLeU32 operation.
    writeLeU32(out, static_cast<std::uint32_t>(state.particles.size()));
    /// Description: Executes the writeLeF32 operation.
    writeLeF32(out, state.totalTime);
    /// Description: Executes the writeLeF32 operation.
    writeLeF32(out, state.config.dt);
    /// Description: Executes the writeLeF32 operation.
    writeLeF32(out, state.config.substepTargetDt);
    /// Description: Executes the writeLeU32 operation.
    writeLeU32(out, state.config.maxSubsteps);
    /// Description: Executes the writeLeU32 operation.
    writeLeU32(out, state.config.snapshotPublishPeriodMs);
    /// Description: Executes the writeLeF32 operation.
    writeLeF32(out, state.config.octreeTheta);
    /// Description: Executes the writeLeF32 operation.
    writeLeF32(out, state.config.octreeSoftening);
    /// Description: Executes the writeLeF32 operation.
    writeLeF32(out, state.config.octreeThetaAutoMin);
    /// Description: Executes the writeLeF32 operation.
    writeLeF32(out, state.config.octreeThetaAutoMax);
    /// Description: Executes the writeLeF32 operation.
    writeLeF32(out, state.config.sphSmoothingLength);
    /// Description: Executes the writeLeF32 operation.
    writeLeF32(out, state.config.sphRestDensity);
    /// Description: Executes the writeLeF32 operation.
    writeLeF32(out, state.config.sphGasConstant);
    /// Description: Executes the writeLeF32 operation.
    writeLeF32(out, state.config.sphViscosity);
    /// Description: Executes the writeLeU32 operation.
    writeLeU32(out, state.config.energyMeasureEverySteps);
    /// Description: Executes the writeLeU32 operation.
    writeLeU32(out, state.config.energySampleLimit);
    /// Description: Executes the writeLeF32 operation.
    writeLeF32(out, state.config.physicsMaxAcceleration);
    /// Description: Executes the writeLeF32 operation.
    writeLeF32(out, state.config.physicsMinSoftening);
    /// Description: Executes the writeLeF32 operation.
    writeLeF32(out, state.config.physicsMinDistance2);
    /// Description: Executes the writeLeF32 operation.
    writeLeF32(out, state.config.physicsMinTheta);
    /// Description: Executes the writeLeF32 operation.
    writeLeF32(out, state.config.sphMaxAcceleration);
    /// Description: Executes the writeLeF32 operation.
    writeLeF32(out, state.config.sphMaxSpeed);
    /// Description: Executes the writeLeF32 operation.
    writeLeF32(out, state.energyBaseline);
    if (!writeSizedString(out, state.config.solver) ||
        !writeSizedString(out, state.config.integrator) ||
        !writeSizedString(out, state.config.performanceProfile) ||
        !writeSizedString(out, state.config.octreeOpeningCriterion)) {
        if (outError != nullptr) {
            *outError = "could not write checkpoint metadata strings";
        }
        return false;
    }
    for (const Particle& particle : state.particles) {
        const Vector3 position = particle.getPosition();
        const Vector3 velocity = particle.getVelocity();
        /// Description: Executes the writeLeF32 operation.
        writeLeF32(out, position.x);
        /// Description: Executes the writeLeF32 operation.
        writeLeF32(out, position.y);
        /// Description: Executes the writeLeF32 operation.
        writeLeF32(out, position.z);
        /// Description: Executes the writeLeF32 operation.
        writeLeF32(out, velocity.x);
        /// Description: Executes the writeLeF32 operation.
        writeLeF32(out, velocity.y);
        /// Description: Executes the writeLeF32 operation.
        writeLeF32(out, velocity.z);
        /// Description: Executes the writeLeF32 operation.
        writeLeF32(out, particle.getMass());
        /// Description: Executes the writeLeF32 operation.
        writeLeF32(out, particle.getTemperature());
    }
    if (!out) {
        if (outError != nullptr) {
            *outError = "could not finish checkpoint write";
        }
        return false;
    }
    return true;
}
