/*
 * @file runtime/server/core/SrvDaemonCommands.cpp
 * @brief Runtime daemon observation, lifecycle, and configuration commands.
 */

#include "core/Daemon.hpp"

#include "config/core/CfgConfig.hpp"
#include "config/directive/CfgConfig.hpp"
#include "config/modes/CfgNormalize.hpp"
#include "config/profile/CfgPerformance.hpp"
#include "protocol/PtcProtocol.hpp"
#include "protocol/codec/PtcJsonCodec.hpp"
#include "SrvSimulationInitConfig.hpp"
#include "SrvSimulationServer.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>

static std::string trimCommandText(const std::string& input)
{
    const auto begin = std::find_if_not(input.begin(), input.end(), [](unsigned char c) {
        return std::isspace(c) != 0;
    });
    const auto end = std::find_if_not(input.rbegin(), input.rend(), [](unsigned char c) {
                         return std::isspace(c) != 0;
                     }).base();
    if (begin >= end) {
        return {};
    }
    return std::string(begin, end);
}

std::optional<std::string> Daemon::processObservationCommand(const std::string& request,
                                                             const std::string& command)
{
    if (command == bltzr_protocol::Status) {
        return bltzr_protocol::JsonCodec::makeStatusResponse(_server.getStats());
    }
    if (command == bltzr_protocol::GetSnapshot) {
        std::uint32_t maxPoints = bltzr_protocol::kSnapshotDefaultPoints;
        bltzr_protocol::JsonCodec::readNumber(request, "max_points", maxPoints);
        maxPoints = bltzr_protocol::clampSnapshotPoints(maxPoints);
        std::vector<RenderParticle> snapshot;
        std::size_t sourceSize = 0u;
        const bool hasSnapshot =
            _server.copyLatestSnapshot(snapshot, static_cast<std::size_t>(maxPoints), &sourceSize);
        return bltzr_protocol::JsonCodec::makeSnapshotResponse(hasSnapshot, snapshot, sourceSize);
    }
    return std::nullopt;
}

std::optional<std::string> Daemon::processLifecycleCommand(const std::string& request,
                                                           const std::string& command)
{
    if (command == bltzr_protocol::Pause) {
        _server.setPaused(true);
        return bltzr_protocol::JsonCodec::makeOkResponse(command);
    }
    if (command == bltzr_protocol::Resume) {
        _server.setPaused(false);
        return bltzr_protocol::JsonCodec::makeOkResponse(command);
    }
    if (command == bltzr_protocol::Toggle) {
        _server.togglePaused();
        return bltzr_protocol::JsonCodec::makeOkResponse(command);
    }
    if (command == bltzr_protocol::Reset) {
        _server.requestReset();
        return bltzr_protocol::JsonCodec::makeOkResponse(command);
    }
    if (command == bltzr_protocol::Recover) {
        _server.requestRecover();
        return bltzr_protocol::JsonCodec::makeOkResponse(command);
    }
    if (command == bltzr_protocol::Step) {
        int count = 1;
        bltzr_protocol::JsonCodec::readNumber(request, "count", count);
        count = std::clamp(count, 1, 100000);
        for (int index = 0; index < count; ++index) {
            _server.stepOnce();
        }
        return bltzr_protocol::JsonCodec::makeOkResponse(command);
    }
    if (command == bltzr_protocol::Shutdown) {
        _shutdownRequested.store(true);
        return bltzr_protocol::JsonCodec::makeOkResponse(command);
    }
    return std::nullopt;
}

std::optional<std::string> Daemon::processCoreConfigurationCommand(const std::string& request,
                                                                   const std::string& command)
{
    if (command == bltzr_protocol::SetInitialStateConfig) {
        std::string serialized;
        if (!bltzr_protocol::JsonCodec::readString(request, "config", serialized) ||
            serialized.empty()) {
            return bltzr_protocol::JsonCodec::makeErrorResponse(
                command, "missing initial state configuration");
        }
        SimulationConfig config = SimulationConfig::defaults();
        std::istringstream input(serialized);
        std::ostringstream warnings;
        std::string line;
        while (std::getline(input, line)) {
            const std::string directive = trimCommandText(line);
            if (directive.empty() || directive.front() == '#') {
                continue;
            }
            if (!bltzr_config::SimulationConfigDirective::applyLine(directive, config, warnings)) {
                return bltzr_protocol::JsonCodec::makeErrorResponse(
                    command, "invalid initial state configuration directive");
            }
        }
        std::ostringstream resolutionLog;
        const ResolvedInitialStatePlan plan = resolveInitialStatePlan(config, resolutionLog);
        if (plan.config.mode == "file") {
            return bltzr_protocol::JsonCodec::makeErrorResponse(
                command, "file initial states must use load");
        }
        _server.setInitialStateConfig(plan.config);
        return bltzr_protocol::JsonCodec::makeOkResponse(command);
    }
    if (command == bltzr_protocol::SetDt) {
        double dt = 0.0;
        if (!bltzr_protocol::JsonCodec::readNumber(request, "value", dt) || dt <= 0.0) {
            return bltzr_protocol::JsonCodec::makeErrorResponse(command, "invalid dt value");
        }
        _server.setDt(static_cast<float>(dt));
        return bltzr_protocol::JsonCodec::makeOkResponse(command);
    }
    if (command == bltzr_protocol::SetSolver) {
        std::string value;
        if (!bltzr_protocol::JsonCodec::readString(request, "value", value)) {
            return bltzr_protocol::JsonCodec::makeErrorResponse(command, "missing solver value");
        }
        std::string canonical;
        if (!bltzr_modes::normalizeSolver(value, canonical)) {
            return bltzr_protocol::JsonCodec::makeErrorResponse(command, "invalid solver value");
        }
        if (!bltzr_modes::isSupportedSolverIntegratorPair(canonical,
                                                          _server.getStats().integratorName)) {
            return bltzr_protocol::JsonCodec::makeErrorResponse(
                command,
                "unsupported solver/integrator combination: octree_gpu does not support rk4");
        }
        _server.setSolverMode(canonical);
        return bltzr_protocol::JsonCodec::makeOkResponse(command);
    }
    if (command == bltzr_protocol::SetIntegrator) {
        std::string value;
        if (!bltzr_protocol::JsonCodec::readString(request, "value", value)) {
            return bltzr_protocol::JsonCodec::makeErrorResponse(command,
                                                                "missing integrator value");
        }
        std::string canonical;
        if (!bltzr_modes::normalizeIntegrator(value, canonical)) {
            return bltzr_protocol::JsonCodec::makeErrorResponse(command,
                                                                "invalid integrator value");
        }
        if (!bltzr_modes::isSupportedSolverIntegratorPair(_server.getStats().solverName,
                                                          canonical)) {
            return bltzr_protocol::JsonCodec::makeErrorResponse(
                command,
                "unsupported solver/integrator combination: octree_gpu does not support rk4");
        }
        _server.setIntegratorMode(canonical);
        return bltzr_protocol::JsonCodec::makeOkResponse(command);
    }
    if (command == bltzr_protocol::SetPerformanceProfile) {
        std::string value;
        if (!bltzr_protocol::JsonCodec::readString(request, "value", value)) {
            return bltzr_protocol::JsonCodec::makeErrorResponse(command,
                                                                "missing performance profile");
        }
        _server.setPerformanceProfile(value);
        return bltzr_protocol::JsonCodec::makeOkResponse(command);
    }
    return std::nullopt;
}

std::optional<std::string> Daemon::processTreePmCommand(const std::string& request,
                                                        const std::string& command)
{
    if (command == bltzr_protocol::SetTreePmAssignment) {
        std::string value;
        if (!bltzr_protocol::JsonCodec::readString(request, "value", value)) {
            return bltzr_protocol::JsonCodec::makeErrorResponse(command,
                                                                "missing TreePM assignment");
        }
        std::string canonical;
        if (!bltzr_config::normalizeTreePmAssignment(value, canonical)) {
            return bltzr_protocol::JsonCodec::makeErrorResponse(
                command, "invalid TreePM assignment; expected cic, tsc, or pcs");
        }
        _server.setTreePmAssignment(canonical);
        return bltzr_protocol::JsonCodec::makeOkResponse(command);
    }
    if (command != bltzr_protocol::SetTreePmParameters) {
        return std::nullopt;
    }

    bool enabled = false;
    bool localGrid = true;
    bool gravityOnlyBuffers = true;
    std::string model;
    std::string layout;
    std::string precision;
    std::string assignment;
    std::uint64_t gridSize = 0u;
    std::uint64_t jacobiIterations = 0u;
    std::uint64_t maxLocalNeighbors = 0u;
    std::uint64_t particleLimit = 0u;
    std::uint64_t denseCellThreshold = 0u;
    double cutoffFactor = 0.0;
    if (!bltzr_protocol::JsonCodec::readBool(request, "enabled", enabled) ||
        !bltzr_protocol::JsonCodec::readString(request, "model", model) ||
        !bltzr_protocol::JsonCodec::readString(request, "precision", precision) ||
        !bltzr_protocol::JsonCodec::readString(request, "assignment", assignment) ||
        !bltzr_protocol::JsonCodec::readBool(request, "local_grid", localGrid) ||
        !bltzr_protocol::JsonCodec::readNumber(request, "grid_size", gridSize) ||
        !bltzr_protocol::JsonCodec::readNumber(request, "jacobi_iters", jacobiIterations) ||
        !bltzr_protocol::JsonCodec::readNumber(request, "cutoff_factor", cutoffFactor) ||
        !bltzr_protocol::JsonCodec::readNumber(request, "max_local_neighbors", maxLocalNeighbors) ||
        !bltzr_protocol::JsonCodec::readNumber(request, "particle_limit", particleLimit) ||
        !bltzr_protocol::JsonCodec::readNumber(request, "dense_cell_threshold",
                                               denseCellThreshold) ||
        !bltzr_protocol::JsonCodec::readBool(request, "gravity_only_buffers", gravityOnlyBuffers) ||
        gridSize > 256u || jacobiIterations > 128u || maxLocalNeighbors > 256u ||
        particleLimit > 100000000u || denseCellThreshold > 4096u || cutoffFactor < 0.0 ||
        cutoffFactor > 8.0) {
        return bltzr_protocol::JsonCodec::makeErrorResponse(command, "invalid TreePM parameters");
    }
    if (!bltzr_protocol::JsonCodec::readString(request, "layout", layout)) {
        layout = "auto";
    }
    _server.setTreePmParameters(
        enabled, model, layout, precision, assignment, localGrid,
        static_cast<std::uint32_t>(gridSize), static_cast<std::uint32_t>(jacobiIterations),
        static_cast<float>(cutoffFactor), static_cast<std::uint32_t>(maxLocalNeighbors),
        static_cast<std::uint32_t>(particleLimit), static_cast<std::uint32_t>(denseCellThreshold),
        gravityOnlyBuffers);
    return bltzr_protocol::JsonCodec::makeOkResponse(command);
}
