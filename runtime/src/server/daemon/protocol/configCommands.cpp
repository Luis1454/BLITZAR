/*
 * @file runtime/src/server/daemon/protocol/configCommands.cpp
 * @brief Handle daemon protocol commands that mutate simulation configuration.
 */

#include "server/ServerDaemon.hpp"
#include "server/daemon/protocol/dispatcher.hpp"
#include "engine/include/server/SimulationServer.hpp"
#include "config/SimulationModes.hpp"
#include "protocol/ServerJsonCodec.hpp"
#include "protocol/ServerProtocol.hpp"
#include <cstdint>
#include <string>

std::string dispatchDaemonConfigCommand(ServerDaemon& daemon,
                                        const DaemonProtocolRequest& parsedRequest)
{
    const std::string& cmd = parsedRequest.envelope.cmd;

    if (cmd == bltzr_protocol::SetDt) {
        double dt = 0.0;
        if (!bltzr_protocol::ServerJsonCodec::readNumber(parsedRequest.rawRequest, "value", dt) ||
            dt <= 0.0) {
            return bltzr_protocol::ServerJsonCodec::makeErrorResponse(cmd, "invalid dt value");
        }
        daemon._server.setDt(static_cast<float>(dt));
        return bltzr_protocol::ServerJsonCodec::makeOkResponse(cmd);
    }
    if (cmd == bltzr_protocol::SetSolver) {
        std::string value;
        if (!bltzr_protocol::ServerJsonCodec::readString(parsedRequest.rawRequest, "value", value)) {
            return bltzr_protocol::ServerJsonCodec::makeErrorResponse(cmd,
                                                                     "missing solver value");
        }
        std::string canonical;
        if (!bltzr_modes::normalizeSolver(value, canonical)) {
            return bltzr_protocol::ServerJsonCodec::makeErrorResponse(cmd,
                                                                     "invalid solver value");
        }
        if (!bltzr_modes::isSupportedSolverIntegratorPair(canonical,
                                                          daemon._server.getStats().integratorName)) {
            return bltzr_protocol::ServerJsonCodec::makeErrorResponse(
                cmd, "unsupported solver/integrator combination: octree_gpu requires euler");
        }
        daemon._server.setSolverMode(canonical);
        return bltzr_protocol::ServerJsonCodec::makeOkResponse(cmd);
    }
    if (cmd == bltzr_protocol::SetIntegrator) {
        std::string value;
        if (!bltzr_protocol::ServerJsonCodec::readString(parsedRequest.rawRequest, "value", value)) {
            return bltzr_protocol::ServerJsonCodec::makeErrorResponse(
                cmd, "missing integrator value");
        }
        std::string canonical;
        if (!bltzr_modes::normalizeIntegrator(value, canonical)) {
            return bltzr_protocol::ServerJsonCodec::makeErrorResponse(
                cmd, "invalid integrator value");
        }
        if (!bltzr_modes::isSupportedSolverIntegratorPair(daemon._server.getStats().solverName,
                                                          canonical)) {
            return bltzr_protocol::ServerJsonCodec::makeErrorResponse(
                cmd, "unsupported solver/integrator combination: octree_gpu requires euler");
        }
        daemon._server.setIntegratorMode(canonical);
        return bltzr_protocol::ServerJsonCodec::makeOkResponse(cmd);
    }
    if (cmd == bltzr_protocol::SetPerformanceProfile) {
        std::string value;
        if (!bltzr_protocol::ServerJsonCodec::readString(parsedRequest.rawRequest, "value", value)) {
            return bltzr_protocol::ServerJsonCodec::makeErrorResponse(
                cmd, "missing performance profile");
        }
        daemon._server.setPerformanceProfile(value);
        return bltzr_protocol::ServerJsonCodec::makeOkResponse(cmd);
    }
    if (cmd == bltzr_protocol::SetParticleCount) {
        std::uint64_t value = 0;
        if (!bltzr_protocol::ServerJsonCodec::readNumber(parsedRequest.rawRequest, "value", value) ||
            value < 2ull) {
            return bltzr_protocol::ServerJsonCodec::makeErrorResponse(cmd,
                                                                     "invalid particle count");
        }
        const std::uint64_t clamped = (value > 100000000ull) ? 100000000ull : value;
        daemon._server.setParticleCount(static_cast<std::uint32_t>(clamped));
        return bltzr_protocol::ServerJsonCodec::makeOkResponse(cmd);
    }
    if (cmd == bltzr_protocol::SetSph) {
        bool value = false;
        if (!bltzr_protocol::ServerJsonCodec::readBool(parsedRequest.rawRequest, "value", value)) {
            return bltzr_protocol::ServerJsonCodec::makeErrorResponse(
                cmd, "missing bool sph value");
        }
        daemon._server.setSphEnabled(value);
        return bltzr_protocol::ServerJsonCodec::makeOkResponse(cmd);
    }
    if (cmd == bltzr_protocol::SetOctree) {
        double theta = 0.0;
        double softening = 0.0;
        if (!bltzr_protocol::ServerJsonCodec::readNumber(parsedRequest.rawRequest, "theta", theta) ||
            theta <= 0.0) {
            return bltzr_protocol::ServerJsonCodec::makeErrorResponse(cmd, "invalid theta");
        }
        if (!bltzr_protocol::ServerJsonCodec::readNumber(parsedRequest.rawRequest, "softening",
                                                         softening) ||
            softening <= 0.0) {
            return bltzr_protocol::ServerJsonCodec::makeErrorResponse(cmd, "invalid softening");
        }
        daemon._server.setOctreeParameters(static_cast<float>(theta), static_cast<float>(softening));
        return bltzr_protocol::ServerJsonCodec::makeOkResponse(cmd);
    }
    if (cmd == bltzr_protocol::SetSphParams) {
        double h = 0.0;
        double restDensity = 0.0;
        double gasConstant = 0.0;
        double viscosity = 0.0;
        if (!bltzr_protocol::ServerJsonCodec::readNumber(parsedRequest.rawRequest, "h", h) ||
            h <= 0.0 ||
            !bltzr_protocol::ServerJsonCodec::readNumber(parsedRequest.rawRequest, "rest_density",
                                                         restDensity) ||
            restDensity <= 0.0 ||
            !bltzr_protocol::ServerJsonCodec::readNumber(parsedRequest.rawRequest, "gas_constant",
                                                         gasConstant) ||
            gasConstant <= 0.0 ||
            !bltzr_protocol::ServerJsonCodec::readNumber(parsedRequest.rawRequest, "viscosity",
                                                         viscosity) ||
            viscosity < 0.0) {
            return bltzr_protocol::ServerJsonCodec::makeErrorResponse(cmd,
                                                                     "invalid sph params");
        }
        daemon._server.setSphParameters(static_cast<float>(h), static_cast<float>(restDensity),
                                        static_cast<float>(gasConstant), static_cast<float>(viscosity));
        return bltzr_protocol::ServerJsonCodec::makeOkResponse(cmd);
    }
    if (cmd == bltzr_protocol::SetSubsteps) {
        double targetDt = 0.0;
        std::uint32_t maxSubsteps = 0;
        if (!bltzr_protocol::ServerJsonCodec::readNumber(parsedRequest.rawRequest, "target_dt",
                                                         targetDt) ||
            targetDt < 0.0 ||
            !bltzr_protocol::ServerJsonCodec::readNumber(parsedRequest.rawRequest, "max_substeps",
                                                         maxSubsteps) ||
            maxSubsteps < 1u) {
            return bltzr_protocol::ServerJsonCodec::makeErrorResponse(cmd,
                                                                     "invalid substep policy");
        }
        daemon._server.setSubstepPolicy(static_cast<float>(targetDt), maxSubsteps);
        return bltzr_protocol::ServerJsonCodec::makeOkResponse(cmd);
    }
    if (cmd == bltzr_protocol::SetEnergyMeasure) {
        std::uint32_t everySteps = 0;
        std::uint32_t sampleLimit = 0;
        if (!bltzr_protocol::ServerJsonCodec::readNumber(parsedRequest.rawRequest, "every_steps",
                                                         everySteps) ||
            everySteps < 1u ||
            !bltzr_protocol::ServerJsonCodec::readNumber(parsedRequest.rawRequest, "sample_limit",
                                                         sampleLimit) ||
            sampleLimit < 2u) {
            return bltzr_protocol::ServerJsonCodec::makeErrorResponse(
                cmd, "invalid energy measure config");
        }
        daemon._server.setEnergyMeasurementConfig(everySteps, sampleLimit);
        return bltzr_protocol::ServerJsonCodec::makeOkResponse(cmd);
    }
    if (cmd == bltzr_protocol::SetGpuTelemetry) {
        bool enabled = false;
        if (!bltzr_protocol::ServerJsonCodec::readBool(parsedRequest.rawRequest, "value",
                                                       enabled)) {
            return bltzr_protocol::ServerJsonCodec::makeErrorResponse(
                cmd, "missing bool gpu telemetry value");
        }
        daemon._server.setGpuTelemetryEnabled(enabled);
        return bltzr_protocol::ServerJsonCodec::makeOkResponse(cmd);
    }
    if (cmd == bltzr_protocol::SetSnapshotPublishCadence) {
        std::uint32_t periodMs = 0;
        if (!bltzr_protocol::ServerJsonCodec::readNumber(parsedRequest.rawRequest, "period_ms",
                                                         periodMs) ||
            periodMs < 1u) {
            return bltzr_protocol::ServerJsonCodec::makeErrorResponse(cmd,
                                                                     "invalid snapshot cadence");
        }
        daemon._server.setSnapshotPublishPeriodMs(periodMs);
        return bltzr_protocol::ServerJsonCodec::makeOkResponse(cmd);
    }
    if (cmd == bltzr_protocol::SetSnapshotTransferCap) {
        std::uint32_t maxPoints = 0;
        if (!bltzr_protocol::ServerJsonCodec::readNumber(parsedRequest.rawRequest, "max_points",
                                                         maxPoints) ||
            maxPoints < 1u) {
            return bltzr_protocol::ServerJsonCodec::makeErrorResponse(
                cmd, "invalid snapshot transfer cap");
        }
        daemon._server.setSnapshotTransferCap(maxPoints);
        return bltzr_protocol::ServerJsonCodec::makeOkResponse(cmd);
    }
    if (cmd == bltzr_protocol::Load) {
        std::string path;
        if (!bltzr_protocol::ServerJsonCodec::readString(parsedRequest.rawRequest, "path", path) ||
            path.empty()) {
            return bltzr_protocol::ServerJsonCodec::makeErrorResponse(cmd, "missing path");
        }
        std::string format = "auto";
        bltzr_protocol::ServerJsonCodec::readString(parsedRequest.rawRequest, "format", format);
        daemon._server.setInitialStateFile(path, format);
        daemon._server.requestReset();
        return bltzr_protocol::ServerJsonCodec::makeOkResponse(cmd);
    }
    if (cmd == bltzr_protocol::Export) {
        std::string path;
        std::string format;
        bltzr_protocol::ServerJsonCodec::readString(parsedRequest.rawRequest, "path", path);
        if (!bltzr_protocol::ServerJsonCodec::readString(parsedRequest.rawRequest, "format",
                                                         format)) {
            format = "vtk";
        }
        daemon._server.requestExportSnapshot(path, format);
        return bltzr_protocol::ServerJsonCodec::makeOkResponse(cmd);
    }
    if (cmd == bltzr_protocol::SaveCheckpoint) {
        std::string path;
        if (!bltzr_protocol::ServerJsonCodec::readString(parsedRequest.rawRequest, "path", path) ||
            path.empty()) {
            return bltzr_protocol::ServerJsonCodec::makeErrorResponse(cmd, "missing path");
        }
        if (!daemon._server.saveCheckpoint(path)) {
            return bltzr_protocol::ServerJsonCodec::makeErrorResponse(
                cmd, "failed to save checkpoint");
        }
        return bltzr_protocol::ServerJsonCodec::makeOkResponse(cmd);
    }
    if (cmd == bltzr_protocol::LoadCheckpoint) {
        std::string path;
        if (!bltzr_protocol::ServerJsonCodec::readString(parsedRequest.rawRequest, "path", path) ||
            path.empty()) {
            return bltzr_protocol::ServerJsonCodec::makeErrorResponse(cmd, "missing path");
        }
        std::string error;
        if (!daemon._server.loadCheckpoint(path, &error)) {
            return bltzr_protocol::ServerJsonCodec::makeErrorResponse(
                cmd, error.empty() ? "failed to load checkpoint" : error);
        }
        return bltzr_protocol::ServerJsonCodec::makeOkResponse(cmd);
    }
    if (cmd == bltzr_protocol::Shutdown) {
        daemon._shutdownRequested.store(true);
        return bltzr_protocol::ServerJsonCodec::makeOkResponse(cmd);
    }
    return {};
}
