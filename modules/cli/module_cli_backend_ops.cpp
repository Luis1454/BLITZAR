#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include "config/TextParse.hpp"
#include "frontend/ErrorBuffer.hpp"
#include "protocol/BackendProtocol.hpp"
#include "modules/cli/module_cli_backend_ops.hpp"

namespace grav_module_cli {

class ModuleCliBackendOpsLocal final {
public:
    static bool ensureConnected(ModuleState &state, char *errorBuffer, std::size_t errorBufferSize)
    {
        if (state.client.isConnected()) {
            return true;
        }
        if (!state.client.connect(state.host, state.port)) {
            grav_frontend::writeErrorBuffer(
                errorBuffer,
                errorBufferSize,
                "unable to connect to backend " + state.host + ":" + std::to_string(state.port));
            return false;
        }
        return true;
    }

    static bool sendSimpleCommand(
        ModuleState &state,
        const std::string &cmd,
        char *errorBuffer,
        std::size_t errorBufferSize)
    {
        if (!ensureConnected(state, errorBuffer, errorBufferSize)) {
            return false;
        }
        const BackendClientResponse response = state.client.sendCommand(cmd);
        if (!response.ok) {
            const std::string message = response.error.empty() ? "backend command failed" : response.error;
            grav_frontend::writeErrorBuffer(errorBuffer, errorBufferSize, message);
            if (response.error == "not connected") {
                state.client.disconnect();
            }
            return false;
        }
        return true;
    }

    static bool commandStatus(ModuleState &state, char *errorBuffer, std::size_t errorBufferSize)
    {
        if (!ensureConnected(state, errorBuffer, errorBufferSize)) {
            return false;
        }
        BackendClientStatus status{};
        const BackendClientResponse response = state.client.getStatus(status);
        if (!response.ok) {
            const std::string message = response.error.empty() ? "status failed" : response.error;
            grav_frontend::writeErrorBuffer(errorBuffer, errorBufferSize, message);
            if (response.error == "not connected") {
                state.client.disconnect();
            }
            return false;
        }
        std::cout << "[module-cli] " << (status.faulted ? "FAULT" : (status.paused ? "PAUSED" : "RUNNING"))
                  << " step=" << status.steps << " dt=" << status.dt
                  << " solver=" << status.solver << " integrator=" << status.integrator
                  << " sph=" << (status.sphEnabled ? "on" : "off")
                  << " sps=" << status.backendFps << " particles=" << status.particleCount
                  << " Etot=" << status.totalEnergy << " dE=" << status.energyDriftPct
                  << " fault_step=" << status.faultStep
                  << (status.faultReason.empty() ? "" : " fault=\"" + status.faultReason + "\"")
                  << (status.energyEstimated ? " est" : "") << "\n";
        return true;
    }

    static bool commandStep(
        ModuleState &state,
        const std::vector<std::string> &tokens,
        char *errorBuffer,
        std::size_t errorBufferSize)
    {
        int count = 1;
        if (tokens.size() >= 2u) {
            if (!grav_text::parseNumber(tokens[1], count) || count < 1) {
                grav_frontend::writeErrorBuffer(errorBuffer, errorBufferSize, "invalid step count");
                return false;
            }
        }
        if (!ensureConnected(state, errorBuffer, errorBufferSize)) {
            return false;
        }
        const BackendClientResponse response = state.client.sendCommand(
            std::string(grav_protocol::Step),
            "\"count\":" + std::to_string(count));
        if (!response.ok) {
            const std::string message = response.error.empty() ? "step failed" : response.error;
            grav_frontend::writeErrorBuffer(errorBuffer, errorBufferSize, message);
            if (response.error == "not connected") {
                state.client.disconnect();
            }
            return false;
        }
        return true;
    }

    static bool connect(
        ModuleState &state,
        const std::vector<std::string> &tokens,
        char *errorBuffer,
        std::size_t errorBufferSize)
    {
        if (tokens.size() < 3u) {
            grav_frontend::writeErrorBuffer(errorBuffer, errorBufferSize, "usage: connect <host> <port>");
            return false;
        }
        unsigned int parsedPort = 0u;
        if (!grav_text::parseNumber(tokens[2], parsedPort) || parsedPort == 0u || parsedPort > 65535u) {
            grav_frontend::writeErrorBuffer(errorBuffer, errorBufferSize, "invalid backend port");
            return false;
        }

        state.host = tokens[1];
        state.port = static_cast<std::uint16_t>(parsedPort);
        state.client.disconnect();
        if (!state.client.connect(state.host, state.port)) {
            grav_frontend::writeErrorBuffer(errorBuffer, errorBufferSize, "connect failed");
            return false;
        }
        std::cout << "[module-cli] connected to " << state.host << ":" << state.port << "\n";
        return true;
    }

    static bool reconnect(ModuleState &state, char *errorBuffer, std::size_t errorBufferSize)
    {
        state.client.disconnect();
        if (!state.client.connect(state.host, state.port)) {
            grav_frontend::writeErrorBuffer(errorBuffer, errorBufferSize, "reconnect failed");
            return false;
        }
        std::cout << "[module-cli] reconnected to " << state.host << ":" << state.port << "\n";
        return true;
    }
};

bool ModuleCliBackendOps::commandStatus(ModuleState &state, char *errorBuffer, std::size_t errorBufferSize)
{
    return ModuleCliBackendOpsLocal::commandStatus(state, errorBuffer, errorBufferSize);
}

bool ModuleCliBackendOps::commandStep(
    ModuleState &state,
    const std::vector<std::string> &tokens,
    char *errorBuffer,
    std::size_t errorBufferSize)
{
    return ModuleCliBackendOpsLocal::commandStep(state, tokens, errorBuffer, errorBufferSize);
}

bool ModuleCliBackendOps::connect(
    ModuleState &state,
    const std::vector<std::string> &tokens,
    char *errorBuffer,
    std::size_t errorBufferSize)
{
    return ModuleCliBackendOpsLocal::connect(state, tokens, errorBuffer, errorBufferSize);
}

bool ModuleCliBackendOps::reconnect(ModuleState &state, char *errorBuffer, std::size_t errorBufferSize)
{
    return ModuleCliBackendOpsLocal::reconnect(state, errorBuffer, errorBufferSize);
}

bool ModuleCliBackendOps::sendSimpleCommand(
    ModuleState &state,
    const std::string &cmd,
    char *errorBuffer,
    std::size_t errorBufferSize)
{
    return ModuleCliBackendOpsLocal::sendSimpleCommand(state, cmd, errorBuffer, errorBufferSize);
}

} // namespace grav_module_cli
