// File: runtime/src/command/CommandExecutor.cpp
// Purpose: Runtime integration surface for BLITZAR clients and protocols.

#include "command/CommandExecutor.hpp"
#include "client/ClientCommon.hpp"
#include "command/CommandCatalog.hpp"
#include "config/SimulationModes.hpp"
#include "config/SimulationPerformanceProfile.hpp"
#include "config/SimulationScenarioValidation.hpp"
#include "protocol/ServerJsonCodec.hpp"
#include "protocol/ServerProtocol.hpp"
#include "server/SimulationInitConfig.hpp"
#include <ostream>
namespace grav_cmd {
static std::string argString(const CommandRequest& request, std::size_t index)
{
    return std::get<std::string>(request.arguments[index]);
}
static std::uint64_t argUint(const CommandRequest& request, std::size_t index)
{
    return std::get<std::uint64_t>(request.arguments[index]);
}
static double argFloat(const CommandRequest& request, std::size_t index)
{
    return std::get<double>(request.arguments[index]);
}
static CommandResult makeFailure(std::string message)
{
    CommandResult result{};
    result.ok = false;
    result.message = std::move(message);
    return result;
}
static CommandResult makeSuccess(std::string message = {})
{
    CommandResult result{};
    result.ok = true;
    result.message = std::move(message);
    return result;
}
static CommandResult sendChecked(CommandExecutionContext& context, const std::string& cmd,
                                 const std::string& fields = {})
{
    if (!context.transport.isConnected() &&
        !context.transport.connect(context.session.host, context.session.port)) {
        return makeFailure("unable to connect to server " + context.session.host + ":" +
                           std::to_string(context.session.port));
    }
    const ServerClientResponse response = context.transport.sendCommand(cmd, fields);
    if (!response.ok)
        return makeFailure(response.error.empty() ? "server command failed" : response.error);
    return makeSuccess();
}
static CommandResult applyConfig(CommandExecutionContext& context, bool requestReset)
{
    const grav_config::ScenarioValidationReport report =
        grav_config::SimulationScenarioValidation::evaluate(context.session.config);
    if (!report.validForRun)
        return makeFailure(grav_config::SimulationScenarioValidation::renderText(report));
    CommandResult result = sendChecked(
        context, std::string(grav_protocol::SetParticleCount),
        "\"value\":" +
            std::to_string(grav_client::resolveServerParticleCount(context.session.config)));
    if (!result.ok)
        return result;
    result = sendChecked(context, std::string(grav_protocol::SetDt),
                         "\"value\":" + std::to_string(context.session.config.dt));
    if (!result.ok)
        return result;
    result = sendChecked(
        context, std::string(grav_protocol::SetSolver),
        "\"value\":\"" +
            grav_protocol::ServerJsonCodec::escapeString(context.session.config.solver) + "\"");
    if (!result.ok)
        return result;
    result = sendChecked(
        context, std::string(grav_protocol::SetIntegrator),
        "\"value\":\"" +
            grav_protocol::ServerJsonCodec::escapeString(context.session.config.integrator) + "\"");
    if (!result.ok)
        return result;
    result = sendChecked(context, std::string(grav_protocol::SetPerformanceProfile),
                         "\"value\":\"" +
                             grav_protocol::ServerJsonCodec::escapeString(
                                 context.session.config.performanceProfile) +
                             "\"");
    if (!result.ok)
        return result;
    result =
        sendChecked(context, std::string(grav_protocol::SetOctree),
                    "\"theta\":" + std::to_string(context.session.config.octreeTheta) +
                        ",\"softening\":" + std::to_string(context.session.config.octreeSoftening));
    if (!result.ok)
        return result;
    result = sendChecked(context, std::string(grav_protocol::SetSph),
                         std::string("\"value\":") +
                             (context.session.config.sphEnabled ? "true" : "false"));
    if (!result.ok)
        return result;
    result = sendChecked(
        context, std::string(grav_protocol::SetSphParams),
        "\"h\":" + std::to_string(context.session.config.sphSmoothingLength) +
            ",\"rest_density\":" + std::to_string(context.session.config.sphRestDensity) +
            ",\"gas_constant\":" + std::to_string(context.session.config.sphGasConstant) +
            ",\"viscosity\":" + std::to_string(context.session.config.sphViscosity));
    if (!result.ok)
        return result;
    result =
        sendChecked(context, std::string(grav_protocol::SetSubsteps),
                    "\"target_dt\":" + std::to_string(context.session.config.substepTargetDt) +
                        ",\"max_substeps\":" + std::to_string(context.session.config.maxSubsteps));
    if (!result.ok)
        return result;
    result = sendChecked(context, std::string(grav_protocol::SetSnapshotPublishCadence),
                         "\"period_ms\":" +
                             std::to_string(context.session.config.snapshotPublishPeriodMs));
    if (!result.ok)
        return result;
    result = sendChecked(
        context, std::string(grav_protocol::SetEnergyMeasure),
        "\"every_steps\":" + std::to_string(context.session.config.energyMeasureEverySteps) +
            ",\"sample_limit\":" + std::to_string(context.session.config.energySampleLimit));
    if (!result.ok)
        return result;
    const ResolvedInitialStatePlan initPlan =
        resolveInitialStatePlan(context.session.config, context.output);
    if (!initPlan.inputFile.empty()) {
        result = sendChecked(context, std::string(grav_protocol::Load),
                             "\"path\":\"" +
                                 grav_protocol::ServerJsonCodec::escapeString(initPlan.inputFile) +
                                 "\",\"format\":\"" +
                                 grav_protocol::ServerJsonCodec::escapeString(
                                     initPlan.inputFormat.empty() ? "auto" : initPlan.inputFormat) +
                                 "\"");
        if (!result.ok)
            return result;
    }
    else if (requestReset)
        result = sendChecked(context, std::string(grav_protocol::Reset));
    if (!result.ok)
        return result;
    context.output << "[command] initial-state templates remain server-owned over the remote API\n";
    return makeSuccess("config applied");
}
static CommandResult renderStatus(CommandExecutionContext& context)
{
    ServerClientStatus status{};
    if (!context.transport.isConnected() &&
        !context.transport.connect(context.session.host, context.session.port)) {
        return makeFailure("unable to connect to server " + context.session.host + ":" +
                           std::to_string(context.session.port));
    }
    const ServerClientResponse response = context.transport.getStatus(status);
    if (!response.ok)
        return makeFailure(response.error.empty() ? "status failed" : response.error);
    context.output << (status.faulted ? "FAULT" : (status.paused ? "PAUSED" : "RUNNING"))
                   << " step=" << status.steps << " dt=" << status.dt << " solver=" << status.solver
                   << " integrator=" << status.integrator << " perf=" << status.performanceProfile
                   << " total_time=" << status.totalTime << " energy=" << status.totalEnergy
                   << " drift=" << status.energyDriftPct << "\n";
    return makeSuccess();
}
static CommandResult runSteps(CommandExecutionContext& context, std::uint64_t stepCount)
{
    CommandResult result = sendChecked(context, std::string(grav_protocol::Pause));
    if (!result.ok)
        return result;
    return sendChecked(context, std::string(grav_protocol::Step),
                       "\"count\":" + std::to_string(stepCount));
}
static CommandResult runUntil(CommandExecutionContext& context, double targetTime)
{
    if (targetTime < 0.0)
        return makeFailure("run_until requires a non-negative simulation time");
    CommandResult result = sendChecked(context, std::string(grav_protocol::Pause));
    if (!result.ok)
        return result;
    for (;;) {
        ServerClientStatus status{};
        const ServerClientResponse statusResponse = context.transport.getStatus(status);
        if (!statusResponse.ok)
            return makeFailure(statusResponse.error.empty() ? "status failed"
                                                            : statusResponse.error);
        if (status.totalTime >= static_cast<float>(targetTime)) {
            return makeSuccess();
        }
        result = sendChecked(context, std::string(grav_protocol::Step), "\"count\":1");
        if (!result.ok)
            return result;
    }
}
CommandResult CommandExecutor::execute(const CommandRequest& request,
                                       CommandExecutionContext& context)
{
    if (request.id == CommandId::Help) {
        context.output << CommandCatalog::renderHelp();
        return makeSuccess();
    }
    if (request.id == CommandId::LoadConfig) {
        context.session.configPath = argString(request, 0u);
        context.session.config = SimulationConfig::loadOrCreate(context.session.configPath);
        return applyConfig(context, true);
    }
    if (request.id == CommandId::Connect) {
        context.session.host = argString(request, 0u);
        context.session.port = static_cast<std::uint16_t>(argUint(request, 1u));
        context.transport.disconnect();
        return context.transport.connect(context.session.host, context.session.port)
                   ? makeSuccess("connected")
                   : makeFailure("connect failed");
    }
    if (request.id == CommandId::Reconnect) {
        context.transport.disconnect();
        return context.transport.connect(context.session.host, context.session.port)
                   ? makeSuccess("reconnected")
                   : makeFailure("reconnect failed");
    }
    if (request.id == CommandId::Status)
        return renderStatus(context);
    if (request.id == CommandId::Pause)
        return sendChecked(context, std::string(grav_protocol::Pause));
    if (request.id == CommandId::Resume)
        return sendChecked(context, std::string(grav_protocol::Resume));
    if (request.id == CommandId::Toggle)
        return sendChecked(context, std::string(grav_protocol::Toggle));
    if (request.id == CommandId::Step)
        return sendChecked(
            context, std::string(grav_protocol::Step),
            "\"count\":" + std::to_string(request.arguments.empty() ? 1u : argUint(request, 0u)));
    if (request.id == CommandId::Reset)
        return sendChecked(context, std::string(grav_protocol::Reset));
    if (request.id == CommandId::Recover)
        return sendChecked(context, std::string(grav_protocol::Recover));
    if (request.id == CommandId::Shutdown)
        return sendChecked(context, std::string(grav_protocol::Shutdown));
    if (request.id == CommandId::SetDt) {
        context.session.config.dt = static_cast<float>(argFloat(request, 0u));
        return sendChecked(context, std::string(grav_protocol::SetDt),
                           "\"value\":" + std::to_string(context.session.config.dt));
    }
    if (request.id == CommandId::SetSolver) {
        std::string canonical;
        if (!grav_modes::normalizeSolver(argString(request, 0u), canonical)) {
            return makeFailure("invalid solver value");
        }
        context.session.config.solver = canonical;
        return sendChecked(context, std::string(grav_protocol::SetSolver),
                           "\"value\":\"" +
                               grav_protocol::ServerJsonCodec::escapeString(canonical) + "\"");
    }
    if (request.id == CommandId::SetIntegrator) {
        std::string canonical;
        if (!grav_modes::normalizeIntegrator(argString(request, 0u), canonical)) {
            return makeFailure("invalid integrator value");
        }
        context.session.config.integrator = canonical;
        return sendChecked(context, std::string(grav_protocol::SetIntegrator),
                           "\"value\":\"" +
                               grav_protocol::ServerJsonCodec::escapeString(canonical) + "\"");
    }
    if (request.id == CommandId::SetProfile) {
        std::string canonical;
        if (!grav_config::normalizePerformanceProfile(argString(request, 0u), canonical)) {
            return makeFailure("invalid performance profile");
        }
        context.session.config.performanceProfile = canonical;
        grav_config::applyPerformanceProfile(context.session.config);
        return applyConfig(context, false);
    }
    if (request.id == CommandId::SetParticleCount) {
        context.session.config.particleCount = static_cast<std::uint32_t>(argUint(request, 0u));
        return sendChecked(context, std::string(grav_protocol::SetParticleCount),
                           "\"value\":" + std::to_string(context.session.config.particleCount));
    }
    if (request.id == CommandId::ExportSnapshot) {
        const std::string path = argString(request, 0u);
        const std::string format = request.arguments.size() >= 2u
                                       ? argString(request, 1u)
                                       : grav_client::inferExportFormatFromPath(path);
        return sendChecked(
            context, std::string(grav_protocol::Export),
            "\"path\":\"" + grav_protocol::ServerJsonCodec::escapeString(path) +
                "\",\"format\":\"" +
                grav_protocol::ServerJsonCodec::escapeString(grav_client::normalizeExportFormat(
                    format.empty() ? context.session.config.exportFormat : format)) +
                "\"");
    }
    if (request.id == CommandId::SaveCheckpoint)
        return sendChecked(
            context, std::string(grav_protocol::SaveCheckpoint),
            "\"path\":\"" + grav_protocol::ServerJsonCodec::escapeString(argString(request, 0u)) +
                "\"");
    if (request.id == CommandId::LoadCheckpoint)
        return sendChecked(
            context, std::string(grav_protocol::LoadCheckpoint),
            "\"path\":\"" + grav_protocol::ServerJsonCodec::escapeString(argString(request, 0u)) +
                "\"");
    if (request.id == CommandId::RunSteps)
        return runSteps(context, argUint(request, 0u));
    return runUntil(context, argFloat(request, 0u));
}
} // namespace grav_cmd
