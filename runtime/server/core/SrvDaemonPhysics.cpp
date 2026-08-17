/*
 * @file runtime/server/core/SrvDaemonPhysics.cpp
 * @brief Runtime daemon adaptive, gravity, fluid, and telemetry commands.
 */

#include "core/Daemon.hpp"

#include "protocol/PtcProtocol.hpp"
#include "protocol/codec/PtcJsonCodec.hpp"
#include "SrvSimulationServer.hpp"

std::optional<std::string> Daemon::processPhysicsCommand(const std::string& request,
                                                         const std::string& command)
{
    if (command == bltzr_protocol::SetAdaptiveTimeSteps) {
        bool enabled = false;
        std::uint64_t maxLevel = 0u;
        double eta = 0.0;
        if (!bltzr_protocol::JsonCodec::readBool(request, "enabled", enabled) ||
            !bltzr_protocol::JsonCodec::readNumber(request, "max_level", maxLevel) ||
            maxLevel > 12u || !bltzr_protocol::JsonCodec::readNumber(request, "eta", eta) ||
            eta < 0.01 || eta > 1.0) {
            return bltzr_protocol::JsonCodec::makeErrorResponse(
                command, "invalid adaptive time-step parameters");
        }
        _server.setAdaptiveTimeStepParameters(enabled, static_cast<std::uint32_t>(maxLevel),
                                              static_cast<float>(eta));
        return bltzr_protocol::JsonCodec::makeOkResponse(command);
    }
    if (command == bltzr_protocol::SetAdaptiveTimeStepCostGuard) {
        bool enabled = true;
        if (!bltzr_protocol::JsonCodec::readBool(request, "enabled", enabled)) {
            return bltzr_protocol::JsonCodec::makeErrorResponse(
                command, "invalid adaptive cost guard value");
        }
        _server.setAdaptiveTimeStepCostGuard(enabled);
        return bltzr_protocol::JsonCodec::makeOkResponse(command);
    }
    if (command == bltzr_protocol::SetParticleCount) {
        std::uint64_t value = 0;
        if (!bltzr_protocol::JsonCodec::readNumber(request, "value", value) || value < 2ull) {
            return bltzr_protocol::JsonCodec::makeErrorResponse(command, "invalid particle count");
        }
        const std::uint64_t clamped = (value > 100000000ull) ? 100000000ull : value;
        _server.setParticleCount(static_cast<std::uint32_t>(clamped));
        return bltzr_protocol::JsonCodec::makeOkResponse(command);
    }
    if (command == bltzr_protocol::SetSph) {
        bool value = false;
        if (!bltzr_protocol::JsonCodec::readBool(request, "value", value)) {
            return bltzr_protocol::JsonCodec::makeErrorResponse(command, "missing bool sph value");
        }
        _server.setSphEnabled(value);
        return bltzr_protocol::JsonCodec::makeOkResponse(command);
    }
    if (command == bltzr_protocol::SetOctree) {
        double theta = 0.0;
        double softening = 0.0;
        if (!bltzr_protocol::JsonCodec::readNumber(request, "theta", theta) || theta <= 0.0) {
            return bltzr_protocol::JsonCodec::makeErrorResponse(command, "invalid theta");
        }
        if (!bltzr_protocol::JsonCodec::readNumber(request, "softening", softening) ||
            softening <= 0.0) {
            return bltzr_protocol::JsonCodec::makeErrorResponse(command, "invalid softening");
        }
        _server.setOctreeParameters(static_cast<float>(theta), static_cast<float>(softening));
        return bltzr_protocol::JsonCodec::makeOkResponse(command);
    }
    if (command == bltzr_protocol::SetSphParams) {
        double h = 0.0;
        double restDensity = 0.0;
        double gasConstant = 0.0;
        double viscosity = 0.0;
        if (!bltzr_protocol::JsonCodec::readNumber(request, "h", h) || h <= 0.0 ||
            !bltzr_protocol::JsonCodec::readNumber(request, "rest_density", restDensity) ||
            restDensity <= 0.0 ||
            !bltzr_protocol::JsonCodec::readNumber(request, "gas_constant", gasConstant) ||
            gasConstant <= 0.0 ||
            !bltzr_protocol::JsonCodec::readNumber(request, "viscosity", viscosity) ||
            viscosity < 0.0) {
            return bltzr_protocol::JsonCodec::makeErrorResponse(command, "invalid sph params");
        }
        _server.setSphParameters(static_cast<float>(h), static_cast<float>(restDensity),
                                 static_cast<float>(gasConstant), static_cast<float>(viscosity));
        return bltzr_protocol::JsonCodec::makeOkResponse(command);
    }
    if (command == bltzr_protocol::SetSubsteps) {
        double targetDt = 0.0;
        std::uint32_t maxSubsteps = 0;
        if (!bltzr_protocol::JsonCodec::readNumber(request, "target_dt", targetDt) ||
            targetDt < 0.0 ||
            !bltzr_protocol::JsonCodec::readNumber(request, "max_substeps", maxSubsteps) ||
            maxSubsteps < 1u) {
            return bltzr_protocol::JsonCodec::makeErrorResponse(command, "invalid substep policy");
        }
        _server.setSubstepPolicy(static_cast<float>(targetDt), maxSubsteps);
        return bltzr_protocol::JsonCodec::makeOkResponse(command);
    }
    if (command == bltzr_protocol::SetEnergyMeasure) {
        std::uint32_t everySteps = 0;
        std::uint32_t sampleLimit = 0;
        if (!bltzr_protocol::JsonCodec::readNumber(request, "every_steps", everySteps) ||
            everySteps < 1u ||
            !bltzr_protocol::JsonCodec::readNumber(request, "sample_limit", sampleLimit) ||
            sampleLimit < 2u) {
            return bltzr_protocol::JsonCodec::makeErrorResponse(command,
                                                                "invalid energy measure config");
        }
        _server.setEnergyMeasurementConfig(everySteps, sampleLimit);
        return bltzr_protocol::JsonCodec::makeOkResponse(command);
    }
    if (command == bltzr_protocol::SetGpuTelemetry) {
        bool enabled = false;
        if (!bltzr_protocol::JsonCodec::readBool(request, "value", enabled)) {
            return bltzr_protocol::JsonCodec::makeErrorResponse(command,
                                                                "missing bool gpu telemetry value");
        }
        _server.setGpuTelemetryEnabled(enabled);
        return bltzr_protocol::JsonCodec::makeOkResponse(command);
    }
    if (command == bltzr_protocol::SetSnapshotPublishCadence) {
        std::uint32_t periodMs = 0;
        if (!bltzr_protocol::JsonCodec::readNumber(request, "period_ms", periodMs) ||
            periodMs < 1u) {
            return bltzr_protocol::JsonCodec::makeErrorResponse(command,
                                                                "invalid snapshot cadence");
        }
        _server.setSnapshotPublishPeriodMs(periodMs);
        return bltzr_protocol::JsonCodec::makeOkResponse(command);
    }
    if (command == bltzr_protocol::SetSnapshotTransferCap) {
        std::uint32_t maxPoints = 0;
        if (!bltzr_protocol::JsonCodec::readNumber(request, "max_points", maxPoints) ||
            maxPoints < 1u) {
            return bltzr_protocol::JsonCodec::makeErrorResponse(command,
                                                                "invalid snapshot transfer cap");
        }
        _server.setSnapshotTransferCap(maxPoints);
        return bltzr_protocol::JsonCodec::makeOkResponse(command);
    }
    return std::nullopt;
}
