/*
 * @file runtime/src/command/execution/Executor.cpp
 * @author BLITZAR Contributors
 * @project BLITZAR
 * @brief Runtime implementation for protocol, command, client, and FFI boundaries.
 */

#include "command/execution/Executor.hpp"
#include "client/common/ClientCommon.hpp"
#include "command/catalog/Catalog.hpp"
#include "config/modes/Normalize.hpp"
#include "config/profile/Performance.hpp"
#include "config/validation/Scenario.hpp"
#include "protocol/codec/JsonCodec.hpp"
#include "protocol/Protocol.hpp"
#include "server/SimulationInitConfig.hpp"
#include <ostream>

namespace bltzr_cmd {
static std::string argString(const Request& request, std::size_t index)
{
    return std::get<std::string>(request.arguments[index]);
}

static std::uint64_t argUint(const Request& request, std::size_t index)
{
    return std::get<std::uint64_t>(request.arguments[index]);
}

static double argFloat(const Request& request, std::size_t index)
{
    return std::get<double>(request.arguments[index]);
}

static Result makeFailure(std::string message)
{
    Result result{};
    result.ok = false;
    result.message = std::move(message);
    return result;
}

static Result makeSuccess(std::string message = {})
{
    Result result{};
    result.ok = true;
    result.message = std::move(message);
    return result;
}

static Result sendChecked(ExecutionContext& context, const std::string& cmd,
                                 const std::string& fields = {})
{
    if (!context.transport.isConnected() &&
        !context.transport.connect(context.session.host, context.session.port)) {
        return makeFailure("unable to connect to server " + context.session.host + ":" +
                           std::to_string(context.session.port));
    }
    const bltzr_protocol::Response response = context.transport.sendCommand(cmd, fields);
    if (!response.ok)
        return makeFailure(response.error.empty() ? "server command failed" : response.error);
    return makeSuccess();
}

static Result applyConfig(ExecutionContext& context, bool requestReset)
{
    const bltzr_config::ScenarioValidationReport report =
        bltzr_config::SimulationScenarioValidation::evaluate(context.session.config);
    if (!report.validForRun)
        return makeFailure(bltzr_config::SimulationScenarioValidation::renderText(report));
    Result result = sendChecked(
        context, std::string(bltzr_protocol::SetParticleCount),
        "\"value\":" +
            std::to_string(bltzr_client::resolveServerParticleCount(context.session.config)));
    if (!result.ok)
        return result;
    result = sendChecked(context, std::string(bltzr_protocol::SetDt),
                         "\"value\":" + std::to_string(context.session.config.dt));
    if (!result.ok)
        return result;
    result = sendChecked(
        context, std::string(bltzr_protocol::SetSolver),
        "\"value\":\"" +
            bltzr_protocol::JsonCodec::escapeString(context.session.config.solver) + "\"");
    if (!result.ok)
        return result;
    result = sendChecked(
        context, std::string(bltzr_protocol::SetIntegrator),
        "\"value\":\"" +
            bltzr_protocol::JsonCodec::escapeString(context.session.config.integrator) +
            "\"");
    if (!result.ok)
        return result;
    result = sendChecked(context, std::string(bltzr_protocol::SetPerformanceProfile),
                         "\"value\":\"" +
                             bltzr_protocol::JsonCodec::escapeString(
                                 context.session.config.performanceProfile) +
                             "\"");
    if (!result.ok)
        return result;
    result =
        sendChecked(context, std::string(bltzr_protocol::SetOctree),
                    "\"theta\":" + std::to_string(context.session.config.octreeTheta) +
                        ",\"softening\":" + std::to_string(context.session.config.octreeSoftening));
    if (!result.ok)
        return result;
    result = sendChecked(context, std::string(bltzr_protocol::SetSph),
                         std::string("\"value\":") +
                             (context.session.config.sphEnabled ? "true" : "false"));
    if (!result.ok)
        return result;
    result = sendChecked(
        context, std::string(bltzr_protocol::SetSphParams),
        "\"h\":" + std::to_string(context.session.config.sphSmoothingLength) +
            ",\"rest_density\":" + std::to_string(context.session.config.sphRestDensity) +
            ",\"gas_constant\":" + std::to_string(context.session.config.sphGasConstant) +
            ",\"viscosity\":" + std::to_string(context.session.config.sphViscosity));
    if (!result.ok)
        return result;
    result =
        sendChecked(context, std::string(bltzr_protocol::SetSubsteps),
                    "\"target_dt\":" + std::to_string(context.session.config.substepTargetDt) +
                        ",\"max_substeps\":" + std::to_string(context.session.config.maxSubsteps));
    if (!result.ok)
        return result;
    result = sendChecked(context, std::string(bltzr_protocol::SetSnapshotPublishCadence),
                         "\"period_ms\":" +
                             std::to_string(context.session.config.snapshotPublishPeriodMs));
    if (!result.ok)
        return result;
    result = sendChecked(
        context, std::string(bltzr_protocol::SetEnergyMeasure),
        "\"every_steps\":" + std::to_string(context.session.config.energyMeasureEverySteps) +
            ",\"sample_limit\":" + std::to_string(context.session.config.energySampleLimit));
    if (!result.ok)
        return result;
    const ResolvedInitialStatePlan initPlan =
        resolveInitialStatePlan(context.session.config, context.output);
    if (!initPlan.inputFile.empty()) {
        result = sendChecked(context, std::string(bltzr_protocol::Load),
                             "\"path\":\"" +
                                 bltzr_protocol::JsonCodec::escapeString(initPlan.inputFile) +
                                 "\",\"format\":\"" +
                                 bltzr_protocol::JsonCodec::escapeString(
                                     initPlan.inputFormat.empty() ? "auto" : initPlan.inputFormat) +
                                 "\"");
        if (!result.ok)
            return result;
    }
    else if (requestReset)
        result = sendChecked(context, std::string(bltzr_protocol::Reset));
    if (!result.ok)
        return result;
    context.output << "[command] initial-state templates remain server-owned over the remote API\n";
    return makeSuccess("config applied");
}

static Result renderStatus(ExecutionContext& context)
{
    bltzr_protocol::ClientStatus status{};
    if (!context.transport.isConnected() &&
        !context.transport.connect(context.session.host, context.session.port)) {
        return makeFailure("unable to connect to server " + context.session.host + ":" +
                           std::to_string(context.session.port));
    }
    const bltzr_protocol::Response response = context.transport.getStatus(status);
    if (!response.ok)
        return makeFailure(response.error.empty() ? "status failed" : response.error);
    context.output << (status.faulted ? "FAULT" : (status.paused ? "PAUSED" : "RUNNING"))
                   << " step=" << status.steps << " dt=" << status.dt << " solver=" << status.solver
                   << " integrator=" << status.integrator << " perf=" << status.performanceProfile
                   << " total_time=" << status.totalTime << " energy=" << status.totalEnergy
                   << " drift=" << status.energyDriftPct << "\n";
    return makeSuccess();
}

static Result runSteps(ExecutionContext& context, std::uint64_t stepCount)
{
    Result result = sendChecked(context, std::string(bltzr_protocol::Pause));
    if (!result.ok)
        return result;
    return sendChecked(context, std::string(bltzr_protocol::Step),
                       "\"count\":" + std::to_string(stepCount));
}

static Result runUntil(ExecutionContext& context, double targetTime)
{
    if (targetTime < 0.0)
        return makeFailure("run_until requires a non-negative simulation time");
    Result result = sendChecked(context, std::string(bltzr_protocol::Pause));
    if (!result.ok)
        return result;
    for (;;) {
        bltzr_protocol::ClientStatus status{};
        const bltzr_protocol::Response statusResponse = context.transport.getStatus(status);
        if (!statusResponse.ok)
            return makeFailure(statusResponse.error.empty() ? "status failed"
                                                            : statusResponse.error);
        if (status.totalTime >= static_cast<float>(targetTime)) {
            return makeSuccess();
        }
        result = sendChecked(context, std::string(bltzr_protocol::Step), "\"count\":1");
        if (!result.ok)
            return result;
    }
}

Result execute(const Request& request, ExecutionContext& context)
{
    if (request.id == Id::Help) {
        context.output << Catalog::renderHelp();
        return makeSuccess();
    }
    if (request.id == Id::LoadConfig) {
        context.session.configPath = argString(request, 0u);
        context.session.config = SimulationConfig::loadOrCreate(context.session.configPath);
        return applyConfig(context, true);
    }
    if (request.id == Id::Connect) {
        context.session.host = argString(request, 0u);
        context.session.port = static_cast<std::uint16_t>(argUint(request, 1u));
        context.transport.disconnect();
        return context.transport.connect(context.session.host, context.session.port)
                   ? makeSuccess("connected")
                   : makeFailure("connect failed");
    }
    if (request.id == Id::Reconnect) {
        context.transport.disconnect();
        return context.transport.connect(context.session.host, context.session.port)
                   ? makeSuccess("reconnected")
                   : makeFailure("reconnect failed");
    }
    if (request.id == Id::Status)
        return renderStatus(context);
    if (request.id == Id::Pause)
        return sendChecked(context, std::string(bltzr_protocol::Pause));
    if (request.id == Id::Resume)
        return sendChecked(context, std::string(bltzr_protocol::Resume));
    if (request.id == Id::Toggle)
        return sendChecked(context, std::string(bltzr_protocol::Toggle));
    if (request.id == Id::Step)
        return sendChecked(
            context, std::string(bltzr_protocol::Step),
            "\"count\":" + std::to_string(request.arguments.empty() ? 1u : argUint(request, 0u)));
    if (request.id == Id::Reset)
        return sendChecked(context, std::string(bltzr_protocol::Reset));
    if (request.id == Id::Recover)
        return sendChecked(context, std::string(bltzr_protocol::Recover));
    if (request.id == Id::Shutdown)
        return sendChecked(context, std::string(bltzr_protocol::Shutdown));
    if (request.id == Id::SetDt) {
        context.session.config.dt = static_cast<float>(argFloat(request, 0u));
        return sendChecked(context, std::string(bltzr_protocol::SetDt),
                           "\"value\":" + std::to_string(context.session.config.dt));
    }
    if (request.id == Id::SetSolver) {
        std::string canonical;
        if (!bltzr_modes::normalizeSolver(argString(request, 0u), canonical)) {
            return makeFailure("invalid solver value");
        }
        context.session.config.solver = canonical;
        return sendChecked(context, std::string(bltzr_protocol::SetSolver),
                           "\"value\":\"" +
                               bltzr_protocol::JsonCodec::escapeString(canonical) + "\"");
    }
    if (request.id == Id::SetIntegrator) {
        std::string canonical;
        if (!bltzr_modes::normalizeIntegrator(argString(request, 0u), canonical)) {
            return makeFailure("invalid integrator value");
        }
        context.session.config.integrator = canonical;
        return sendChecked(context, std::string(bltzr_protocol::SetIntegrator),
                           "\"value\":\"" +
                               bltzr_protocol::JsonCodec::escapeString(canonical) + "\"");
    }
    if (request.id == Id::SetProfile) {
        std::string canonical;
        if (!bltzr_config::normalizePerformanceProfile(argString(request, 0u), canonical)) {
            return makeFailure("invalid performance profile");
        }
        context.session.config.performanceProfile = canonical;
        bltzr_config::applyPerformanceProfile(context.session.config);
        return applyConfig(context, false);
    }
    if (request.id == Id::SetParticleCount) {
        context.session.config.particleCount = static_cast<std::uint32_t>(argUint(request, 0u));
        return sendChecked(context, std::string(bltzr_protocol::SetParticleCount),
                           "\"value\":" + std::to_string(context.session.config.particleCount));
    }
    if (request.id == Id::ExportSnapshot) {
        const std::string path = argString(request, 0u);
        const std::string format = request.arguments.size() >= 2u
                                       ? argString(request, 1u)
                                       : bltzr_client::inferExportFormatFromPath(path);
        return sendChecked(
            context, std::string(bltzr_protocol::Export),
            "\"path\":\"" + bltzr_protocol::JsonCodec::escapeString(path) +
                "\",\"format\":\"" +
                bltzr_protocol::JsonCodec::escapeString(bltzr_client::normalizeExportFormat(
                    format.empty() ? context.session.config.exportFormat : format)) +
                "\"");
    }
    if (request.id == Id::SaveCheckpoint)
        return sendChecked(
            context, std::string(bltzr_protocol::SaveCheckpoint),
            "\"path\":\"" + bltzr_protocol::JsonCodec::escapeString(argString(request, 0u)) +
                "\"");
    if (request.id == Id::LoadCheckpoint)
        return sendChecked(
            context, std::string(bltzr_protocol::LoadCheckpoint),
            "\"path\":\"" + bltzr_protocol::JsonCodec::escapeString(argString(request, 0u)) +
                "\"");
    if (request.id == Id::RunSteps)
        return runSteps(context, argUint(request, 0u));
    return runUntil(context, argFloat(request, 0u));
}
} // namespace bltzr_cmd
