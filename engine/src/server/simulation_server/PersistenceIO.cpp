// File: engine/src/server/simulation_server/PersistenceIO.cpp
// Purpose: Engine implementation for the BLITZAR simulation core.

#include "Internal.hpp"
bool readCheckpointFile(const std::string& inputPath, SimulationCheckpointState& outState,
                        std::string* outError)
{
    /// Description: Executes the in operation.
    std::ifstream in(inputPath, std::ios::binary);
    if (!in.is_open()) {
        if (outError != nullptr) {
            *outError = "could not open checkpoint input";
        }
        return false;
    }
    std::array<std::byte, sizeof(kCheckpointMagic)> magic{};
    if (!readRawBytes(in, magic.data(), magic.size()) ||
        /// Description: Executes the memcmp operation.
        std::memcmp(magic.data(), kCheckpointMagic, sizeof(kCheckpointMagic)) != 0) {
        if (outError != nullptr) {
            *outError = "invalid checkpoint magic";
        }
        return false;
    }
    std::uint32_t version = 0u;
    std::uint32_t flags = 0u;
    std::uint64_t steps = 0u;
    std::uint32_t count = 0u;
    if (!readLeU32(in, version) || version != kCheckpointVersion || !readLeU32(in, flags) ||
        !readLeU64(in, steps) || !readLeU32(in, count) || !isValidImportedParticleCount(count)) {
        if (outError != nullptr) {
            *outError = "unsupported checkpoint version";
        }
        return false;
    }
    outState = SimulationCheckpointState{};
    outState.steps = steps;
    outState.paused = (flags & kCheckpointFlagPaused) != 0u;
    outState.hasEnergyBaseline = (flags & kCheckpointFlagHasEnergyBaseline) != 0u;
    outState.config.sphEnabled = (flags & kCheckpointFlagSphEnabled) != 0u;
    outState.config.octreeThetaAutoTune = (flags & kCheckpointFlagThetaAutoTune) != 0u;
    outState.gpuTelemetryEnabled = (flags & kCheckpointFlagGpuTelemetryEnabled) != 0u;
    outState.config.particleCount = count;
    if (!readLeF32(in, outState.totalTime) || !readLeF32(in, outState.config.dt) ||
        !readLeF32(in, outState.config.substepTargetDt) ||
        !readLeU32(in, outState.config.maxSubsteps) ||
        !readLeU32(in, outState.config.snapshotPublishPeriodMs) ||
        !readLeF32(in, outState.config.octreeTheta) ||
        !readLeF32(in, outState.config.octreeSoftening) ||
        !readLeF32(in, outState.config.octreeThetaAutoMin) ||
        !readLeF32(in, outState.config.octreeThetaAutoMax) ||
        !readLeF32(in, outState.config.sphSmoothingLength) ||
        !readLeF32(in, outState.config.sphRestDensity) ||
        !readLeF32(in, outState.config.sphGasConstant) ||
        !readLeF32(in, outState.config.sphViscosity) ||
        !readLeU32(in, outState.config.energyMeasureEverySteps) ||
        !readLeU32(in, outState.config.energySampleLimit) ||
        !readLeF32(in, outState.config.physicsMaxAcceleration) ||
        !readLeF32(in, outState.config.physicsMinSoftening) ||
        !readLeF32(in, outState.config.physicsMinDistance2) ||
        !readLeF32(in, outState.config.physicsMinTheta) ||
        !readLeF32(in, outState.config.sphMaxAcceleration) ||
        !readLeF32(in, outState.config.sphMaxSpeed) || !readLeF32(in, outState.energyBaseline) ||
        !readSizedString(in, outState.config.solver, 32u) ||
        !readSizedString(in, outState.config.integrator, 32u) ||
        !readSizedString(in, outState.config.performanceProfile, 32u) ||
        !readSizedString(in, outState.config.octreeOpeningCriterion, 32u)) {
        if (outError != nullptr) {
            *outError = "checkpoint metadata is truncated";
        }
        return false;
    }
    if (!isSupportedCheckpointString(outState.config.solver, outState.config.integrator,
                                     outState.config.performanceProfile,
                                     outState.config.octreeOpeningCriterion)) {
        if (outError != nullptr) {
            *outError = "checkpoint metadata is not supported by this build";
        }
        return false;
    }
    outState.particles.clear();
    outState.particles.reserve(count);
    for (std::uint32_t index = 0u; index < count; ++index) {
        float px = 0.0f;
        float py = 0.0f;
        float pz = 0.0f;
        float vx = 0.0f;
        float vy = 0.0f;
        float vz = 0.0f;
        float mass = 0.0f;
        float temperature = 0.0f;
        if (!readLeF32(in, px) || !readLeF32(in, py) || !readLeF32(in, pz) ||
            !readLeF32(in, vx) || !readLeF32(in, vy) || !readLeF32(in, vz) ||
            !readLeF32(in, mass) || !readLeF32(in, temperature)) {
            if (outError != nullptr) {
                *outError = "checkpoint particle payload is truncated";
            }
            return false;
        }
        Particle particle;
        particle.setPosition(Vector3(px, py, pz));
        particle.setVelocity(Vector3(vx, vy, vz));
        particle.setPressure(Vector3(0.0f, 0.0f, 0.0f));
        particle.setDensity(0.0f);
        if (mass > 0.0f) {
            particle.setMass(mass);
        }
        particle.setTemperature(std::max(0.0f, temperature));
        outState.particles.push_back(particle);
    }
    return outState.particles.size() >= 2u;
}
/// Description: Executes the writeExportSnapshotFile operation.
bool writeExportSnapshotFile(const AsyncExportJob& job)
{
    /// Description: Executes the outPath operation.
    std::filesystem::path outPath(job.outputPath);
    if (outPath.has_parent_path()) {
        std::error_code ec;
        /// Description: Executes the create_directories operation.
        std::filesystem::create_directories(outPath.parent_path(), ec);
    }
    const std::string fmt = normalizeSnapshotFormat(job.format);
    if (fmt == "bin") {
        /// Description: Executes the out operation.
        std::ofstream out(job.outputPath, std::ios::binary | std::ios::trunc);
        if (!out.is_open()) {
            return false;
        }
        BinarySnapshotHeader header{};
        /// Description: Executes the memcpy operation.
        std::memcpy(header.magic, kBinarySnapshotMagic, sizeof(kBinarySnapshotMagic));
        header.version = kBinarySnapshotVersion;
        header.count = static_cast<std::uint32_t>(job.particles.size());
        std::array<std::byte, sizeof(BinarySnapshotHeader)> headerBytes{};
        /// Description: Executes the memcpy operation.
        std::memcpy(headerBytes.data(), &header, sizeof(header));
        if (!writeRawBytes(out, headerBytes.data(), headerBytes.size())) {
            return false;
        }
        for (const Particle& particle : job.particles) {
            const Vector3 position = particle.getPosition();
            const Vector3 velocity = particle.getVelocity();
            const BinarySnapshotParticle record{
                position.x, position.y, position.z,         velocity.x,
                velocity.y, velocity.z, particle.getMass(), particle.getTemperature()};
            std::array<std::byte, sizeof(BinarySnapshotParticle)> recordBytes{};
            /// Description: Executes the memcpy operation.
            std::memcpy(recordBytes.data(), &record, sizeof(record));
            if (!writeRawBytes(out, recordBytes.data(), recordBytes.size())) {
                return false;
            }
        }
        return true;
    }
    const bool vtkBinary = (fmt == "vtk_binary");
    std::ofstream out(job.outputPath,
                      (vtkBinary ? (std::ios::binary | std::ios::trunc) : std::ios::trunc));
    if (!out.is_open()) {
        return false;
    }
    if (fmt == "xyz") {
        out << job.particles.size() << "\n";
        out << "solver=" << job.solverModeLabel << " integrator=" << job.integratorModeLabel
            << " step=" << job.step << "\n";
        for (const Particle& particle : job.particles) {
            const Vector3 position = particle.getPosition();
            out << "P " << position.x << " " << position.y << " " << position.z << " "
                << particle.getMass() << " " << particle.getTemperature() << "\n";
        }
        return true;
    }
    out << "# vtk DataFile Version 3.0\n";
    out << "BLITZAR snapshot solver=" << job.solverModeLabel
        << " integrator=" << job.integratorModeLabel << "\n";
    out << (vtkBinary ? "BINARY\n" : "ASCII\n");
    out << "DATASET POLYDATA\n";
    out << "POINTS " << job.particles.size() << " float\n";
    if (vtkBinary) {
        for (const Particle& particle : job.particles) {
            const Vector3 position = particle.getPosition();
            /// Description: Executes the writeBeF32 operation.
            writeBeF32(out, position.x);
            /// Description: Executes the writeBeF32 operation.
            writeBeF32(out, position.y);
            /// Description: Executes the writeBeF32 operation.
            writeBeF32(out, position.z);
        }
        out << "\n";
    }
    else {
        for (const Particle& particle : job.particles) {
            const Vector3 position = particle.getPosition();
            out << position.x << " " << position.y << " " << position.z << "\n";
        }
    }
    out << "VERTICES " << job.particles.size() << " " << (job.particles.size() * 2) << "\n";
    if (vtkBinary) {
        for (std::size_t i = 0; i < job.particles.size(); ++i) {
            /// Description: Executes the writeBeI32 operation.
            writeBeI32(out, 1);
            /// Description: Executes the writeBeI32 operation.
            writeBeI32(out, static_cast<std::int32_t>(i));
        }
        out << "\n";
    }
    else {
        for (std::size_t i = 0; i < job.particles.size(); ++i) {
            out << "1 " << i << "\n";
        }
    }
    out << "POINT_DATA " << job.particles.size() << "\n";
    out << "SCALARS mass float 1\n";
    out << "LOOKUP_TABLE default\n";
    if (vtkBinary) {
        for (const Particle& particle : job.particles) {
            /// Description: Executes the writeBeF32 operation.
            writeBeF32(out, particle.getMass());
        }
        out << "\n";
    }
    else {
        for (const Particle& particle : job.particles) {
            out << particle.getMass() << "\n";
        }
    }
    out << "SCALARS pressure float 1\n";
    out << "LOOKUP_TABLE default\n";
    if (vtkBinary) {
        for (const Particle& particle : job.particles) {
            /// Description: Executes the writeBeF32 operation.
            writeBeF32(out, particle.getPressure().norm());
        }
        out << "\n";
    }
    else {
        for (const Particle& particle : job.particles) {
            out << particle.getPressure().norm() << "\n";
        }
    }
    out << "SCALARS temperature float 1\n";
    out << "LOOKUP_TABLE default\n";
    if (vtkBinary) {
        for (const Particle& particle : job.particles) {
            /// Description: Executes the writeBeF32 operation.
            writeBeF32(out, particle.getTemperature());
        }
        out << "\n";
    }
    else {
        for (const Particle& particle : job.particles) {
            out << particle.getTemperature() << "\n";
        }
    }
    out << "VECTORS velocity float\n";
    if (vtkBinary) {
        for (const Particle& particle : job.particles) {
            const Vector3 velocity = particle.getVelocity();
            /// Description: Executes the writeBeF32 operation.
            writeBeF32(out, velocity.x);
            /// Description: Executes the writeBeF32 operation.
            writeBeF32(out, velocity.y);
            /// Description: Executes the writeBeF32 operation.
            writeBeF32(out, velocity.z);
        }
        out << "\n";
    }
    else {
        for (const Particle& particle : job.particles) {
            const Vector3 velocity = particle.getVelocity();
            out << velocity.x << " " << velocity.y << " " << velocity.z << "\n";
        }
    }
    return true;
}
