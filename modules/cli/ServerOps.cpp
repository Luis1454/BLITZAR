/*
 * @file modules/cli/ServerOps.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Command-line client module for runtime control workflows.
 */

#include "Constants.hpp"
#include "modules/cli/ServerOps.hpp"
#include "client/diagnostics/ErrorBuffer.hpp"
#include "config/text/Parse.hpp"
#include "protocol/Protocol.hpp"
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace bltzr_module_cli {
namespace {
bool ensureConnected(State& state, const bltzr_client::ErrorBufferView& errorBuffer)
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

bool sendSimpleCommandImpl(State& state, const std::string& cmd,
                           const bltzr_client::ErrorBufferView& errorBuffer)
{
    if (!ensureConnected(state, errorBuffer)) {
        return false;
    }
    const bltzr_protocol::Response response = state.transport.sendCommand(cmd);
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

bool commandStatusImpl(State& state, const bltzr_client::ErrorBufferView& errorBuffer)
{
    if (!ensureConnected(state, errorBuffer)) {
        return false;
    }
    bltzr_protocol::ClientStatus status{};
    const bltzr_protocol::Response response = state.transport.getStatus(status);
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

bool commandStepImpl(State& state, const std::vector<std::string>& tokens,
                     const bltzr_client::ErrorBufferView& errorBuffer)
{
    int count = 1;
    if (tokens.size() >= 2u) {
        if (!bltzr_text::parseNumber(tokens[1], count) || count < 1) {
            errorBuffer.write("invalid step count");
            return false;
        }
    }
    if (!ensureConnected(state, errorBuffer)) {
        return false;
    }
    const bltzr_protocol::Response response = state.transport.sendCommand(
        std::string(bltzr_protocol::Step), "\"count\":" + std::to_string(count));
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

bool connectToServer(State& state, const std::vector<std::string>& tokens,
                     const bltzr_client::ErrorBufferView& errorBuffer)
{
    if (tokens.size() < 3u) {
        errorBuffer.write("usage: connect <host> <port>");
        return false;
    }
    unsigned int parsedPort = 0u;
    if (!bltzr_text::parseNumber(tokens[2], parsedPort) || parsedPort < kNetworkPortMin ||
        parsedPort > kNetworkPortMax) {
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

bool reconnectToServer(State& state, const bltzr_client::ErrorBufferView& errorBuffer)
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
} // namespace

bool ServerOps::commandStatus(State& state,
                              const bltzr_client::ErrorBufferView& errorBuffer)
{
    return commandStatusImpl(state, errorBuffer);
}

bool ServerOps::commandStep(State& state, const std::vector<std::string>& tokens,
                            const bltzr_client::ErrorBufferView& errorBuffer)
{
    return commandStepImpl(state, tokens, errorBuffer);
}

bool ServerOps::connect(State& state, const std::vector<std::string>& tokens,
                        const bltzr_client::ErrorBufferView& errorBuffer)
{
    return connectToServer(state, tokens, errorBuffer);
}

bool ServerOps::reconnect(State& state, const bltzr_client::ErrorBufferView& errorBuffer)
{
    return reconnectToServer(state, errorBuffer);
}

bool ServerOps::sendSimpleCommand(State& state, const std::string& cmd,
                                  const bltzr_client::ErrorBufferView& errorBuffer)
{
    return sendSimpleCommandImpl(state, cmd, errorBuffer);
}
} // namespace bltzr_module_cli
