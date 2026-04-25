#include "modules/cli/module_cli_server_ops.hpp"
#include "client/ErrorBuffer.hpp"
#include "config/TextParse.hpp"
#include "protocol/ServerProtocol.hpp"
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>
namespace grav_module_cli {
class ModuleCliServerOpsLocal final {
public:
    static bool ensureConnected(ModuleState& state, const grav_client::ErrorBufferView& errorBuffer)
    {
        if (state.transport.isConnected()) {
            return true;
        }
        if (!state.transport.connect(state.session.host, state.session.port)) {
            errorBuffer.write("unable to connect to server " + state.session.host + ":" +
                              std::to_string(state.session.port));
            return false;
        }
        return true;
    }
    static bool sendSimpleCommand(ModuleState& state, const std::string& cmd,
                                  const grav_client::ErrorBufferView& errorBuffer)
    {
        if (!ensureConnected(state, errorBuffer)) {
            return false;
        }
        const ServerClientResponse response = state.transport.sendCommand(cmd);
        if (!response.ok) {
            const std::string message =
                response.error.empty() ? "server command failed" : response.error;
            errorBuffer.write(message);
            if (response.error == "not connected") {
                state.transport.disconnect();
            }
            return false;
        }
        return true;
    }
    static bool commandStatus(ModuleState& state, const grav_client::ErrorBufferView& errorBuffer)
    {
        if (!ensureConnected(state, errorBuffer)) {
            return false;
        }
        ServerClientStatus status{};
        const ServerClientResponse response = state.transport.getStatus(status);
        if (!response.ok) {
            const std::string message = response.error.empty() ? "status failed" : response.error;
            errorBuffer.write(message);
            if (response.error == "not connected") {
                state.transport.disconnect();
            }
            return false;
        }
        std::cout << "[module-cli] "
                  << (status.faulted ? "FAULT" : (status.paused ? "PAUSED" : "RUNNING"))
                  << " step=" << status.steps << " dt=" << status.dt << "s"
                  << " solver=" << status.solver << " integrator=" << status.integrator
                  << " perf=" << status.performanceProfile
                  << " sph=" << (status.sphEnabled ? "on" : "off") << " sps=" << status.serverFps
                  << " particles=" << status.particleCount << " substeps=" << status.substeps
                  << " subdt=" << status.substepDt << "s"
                  << " target=" << status.substepTargetDt << "s"
                  << " max_substeps=" << status.maxSubsteps
                  << " snapshot=" << status.snapshotPublishPeriodMs << "ms"
                  << " Etot=" << status.totalEnergy << "J dE=" << status.energyDriftPct
                  << " fault_step=" << status.faultStep
                  << (status.faultReason.empty() ? "" : " fault=\"" + status.faultReason + "\"")
                  << (status.energyEstimated ? " est" : "") << "\n";
        return true;
    }
    static bool commandStep(ModuleState& state, const std::vector<std::string>& tokens,
                            const grav_client::ErrorBufferView& errorBuffer)
    {
        int count = 1;
        if (tokens.size() >= 2u) {
            if (!grav_text::parseNumber(tokens[1], count) || count < 1) {
                errorBuffer.write("invalid step count");
                return false;
            }
        }
        if (!ensureConnected(state, errorBuffer)) {
            return false;
        }
        const ServerClientResponse response = state.transport.sendCommand(
            std::string(grav_protocol::Step), "\"count\":" + std::to_string(count));
        if (!response.ok) {
            const std::string message = response.error.empty() ? "step failed" : response.error;
            errorBuffer.write(message);
            if (response.error == "not connected") {
                state.transport.disconnect();
            }
            return false;
        }
        return true;
    }
    static bool connect(ModuleState& state, const std::vector<std::string>& tokens,
                        const grav_client::ErrorBufferView& errorBuffer)
    {
        if (tokens.size() < 3u) {
            errorBuffer.write("usage: connect <host> <port>");
            return false;
        }
        unsigned int parsedPort = 0u;
        if (!grav_text::parseNumber(tokens[2], parsedPort) || parsedPort == 0u ||
            parsedPort > 65535u) {
            errorBuffer.write("invalid server port");
            return false;
        }
        state.session.host = tokens[1];
        state.session.port = static_cast<std::uint16_t>(parsedPort);
        state.transport.disconnect();
        if (!state.transport.connect(state.session.host, state.session.port)) {
            errorBuffer.write("connect failed");
            return false;
        }
        std::cout << "[module-cli] connected to " << state.session.host << ":" << state.session.port
                  << "\n";
        return true;
    }
    static bool reconnect(ModuleState& state, const grav_client::ErrorBufferView& errorBuffer)
    {
        state.transport.disconnect();
        if (!state.transport.connect(state.session.host, state.session.port)) {
            errorBuffer.write("reconnect failed");
            return false;
        }
        std::cout << "[module-cli] reconnected to " << state.session.host << ":"
                  << state.session.port << "\n";
        return true;
    }
};
bool ModuleCliServerOps::commandStatus(ModuleState& state,
                                       const grav_client::ErrorBufferView& errorBuffer)
{
    return ModuleCliServerOpsLocal::commandStatus(state, errorBuffer);
}
bool ModuleCliServerOps::commandStep(ModuleState& state, const std::vector<std::string>& tokens,
                                     const grav_client::ErrorBufferView& errorBuffer)
{
    return ModuleCliServerOpsLocal::commandStep(state, tokens, errorBuffer);
}
bool ModuleCliServerOps::connect(ModuleState& state, const std::vector<std::string>& tokens,
                                 const grav_client::ErrorBufferView& errorBuffer)
{
    return ModuleCliServerOpsLocal::connect(state, tokens, errorBuffer);
}
bool ModuleCliServerOps::reconnect(ModuleState& state,
                                   const grav_client::ErrorBufferView& errorBuffer)
{
    return ModuleCliServerOpsLocal::reconnect(state, errorBuffer);
}
bool ModuleCliServerOps::sendSimpleCommand(ModuleState& state, const std::string& cmd,
                                           const grav_client::ErrorBufferView& errorBuffer)
{
    return ModuleCliServerOpsLocal::sendSimpleCommand(state, cmd, errorBuffer);
}
} // namespace grav_module_cli
