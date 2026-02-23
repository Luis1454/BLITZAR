#include "sim/BackendClient.hpp"
#include "sim/BackendProtocol.hpp"
#include "sim/ErrorBuffer.hpp"
#include "sim/FrontendModuleApi.hpp"
#include "sim/TextParse.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <exception>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

struct ModuleState {
    BackendClient client;
    std::string host = "127.0.0.1";
    std::uint16_t port = 4545u;
};

std::string trim(const std::string &input)
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

std::vector<std::string> splitTokens(const std::string &line)
{
    std::vector<std::string> tokens;
    std::istringstream input(line);
    std::string token;
    while (input >> token) {
        tokens.push_back(token);
    }
    return tokens;
}

bool ensureConnected(ModuleState &state, char *errorBuffer, std::size_t errorBufferSize)
{
    if (state.client.isConnected()) {
        return true;
    }
    if (!state.client.connect(state.host, state.port)) {
        sim::writeErrorBuffer(
            errorBuffer,
            errorBufferSize,
            "unable to connect to backend " + state.host + ":" + std::to_string(state.port));
        return false;
    }
    return true;
}

bool sendSimpleCommand(ModuleState &state, const std::string &cmd, char *errorBuffer, std::size_t errorBufferSize)
{
    if (!ensureConnected(state, errorBuffer, errorBufferSize)) {
        return false;
    }
    const BackendClientResponse response = state.client.sendCommand(cmd);
    if (!response.ok) {
        sim::writeErrorBuffer(errorBuffer, errorBufferSize, response.error.empty() ? "backend command failed" : response.error);
        if (response.error == "not connected") {
            state.client.disconnect();
        }
        return false;
    }
    return true;
}

void printHelp()
{
    std::cout
        << "[module-cli] commands:\n"
        << "  help\n"
        << "  connect <host> <port>\n"
        << "  reconnect\n"
        << "  status\n"
        << "  pause | resume | toggle\n"
        << "  step [count]\n"
        << "  reset | recover\n"
        << "  shutdown\n";
}

bool commandStatus(ModuleState &state, char *errorBuffer, std::size_t errorBufferSize)
{
    if (!ensureConnected(state, errorBuffer, errorBufferSize)) {
        return false;
    }
    BackendClientStatus status{};
    const BackendClientResponse response = state.client.getStatus(status);
    if (!response.ok) {
        sim::writeErrorBuffer(errorBuffer, errorBufferSize, response.error.empty() ? "status failed" : response.error);
        if (response.error == "not connected") {
            state.client.disconnect();
        }
        return false;
    }
    std::cout << "[module-cli] "
              << (status.faulted ? "FAULT" : (status.paused ? "PAUSED" : "RUNNING"))
              << " step=" << status.steps
              << " dt=" << status.dt
              << " solver=" << status.solver
              << " sph=" << (status.sphEnabled ? "on" : "off")
              << " sps=" << status.backendFps
              << " particles=" << status.particleCount
              << " Etot=" << status.totalEnergy
              << " dE=" << status.energyDriftPct
              << " fault_step=" << status.faultStep
              << (status.faultReason.empty() ? "" : " fault=\"" + status.faultReason + "\"")
              << (status.energyEstimated ? " est" : "")
              << "\n";
    return true;
}

bool commandStep(
    ModuleState &state,
    const std::vector<std::string> &tokens,
    char *errorBuffer,
    std::size_t errorBufferSize)
{
    int count = 1;
    if (tokens.size() >= 2u) {
        if (!sim::text::parseNumber(tokens[1], count) || count < 1) {
            sim::writeErrorBuffer(errorBuffer, errorBufferSize, "invalid step count");
            return false;
        }
    }
    if (!ensureConnected(state, errorBuffer, errorBufferSize)) {
        return false;
    }
    const BackendClientResponse response = state.client.sendCommand(
        std::string(sim::protocol::cmd::Step),
        "\"count\":" + std::to_string(count));
    if (!response.ok) {
        sim::writeErrorBuffer(errorBuffer, errorBufferSize, response.error.empty() ? "step failed" : response.error);
        if (response.error == "not connected") {
            state.client.disconnect();
        }
        return false;
    }
    return true;
}

} // namespace

GRAVITY_FRONTEND_MODULE_EXPORT const sim::module::FrontendModuleExportsV1 *gravity_frontend_module_v1()
{
    static const sim::module::FrontendModuleExportsV1 exports{
        sim::module::kFrontendModuleApiVersionV1,
        "cli-module",
        [](const sim::module::FrontendModuleHostContextV1 *, void **outModuleState, char *errorBuffer, std::size_t errorBufferSize) -> bool {
            try {
                if (outModuleState == nullptr) {
                    sim::writeErrorBuffer(errorBuffer, errorBufferSize, "outModuleState is null");
                    return false;
                }
                auto *state = new ModuleState();
                state->client.setSocketTimeoutMs(150);
                *outModuleState = state;
                return true;
            } catch (const std::exception &ex) {
                sim::writeErrorBuffer(errorBuffer, errorBufferSize, ex.what());
                return false;
            } catch (...) {
                sim::writeErrorBuffer(errorBuffer, errorBufferSize, "unknown module create error");
                return false;
            }
        },
        [](void *moduleState) {
            try {
                auto *state = static_cast<ModuleState *>(moduleState);
                if (state != nullptr) {
                    state->client.disconnect();
                    delete state;
                }
            } catch (const std::exception &ex) {
                std::cerr << "[module-cli] destroy error: " << ex.what() << "\n";
            } catch (...) {
                std::cerr << "[module-cli] destroy error: unknown\n";
            }
        },
        [](void *moduleState, char *errorBuffer, std::size_t errorBufferSize) -> bool {
            try {
                auto *state = static_cast<ModuleState *>(moduleState);
                if (state == nullptr) {
                    sim::writeErrorBuffer(errorBuffer, errorBufferSize, "module state is null");
                    return false;
                }
                if (!state->client.connect(state->host, state->port)) {
                    std::cout << "[module-cli] startup: backend " << state->host << ":" << state->port
                              << " not reachable yet (retry on command)\n";
                } else {
                    std::cout << "[module-cli] connected to " << state->host << ":" << state->port << "\n";
                }
                return true;
            } catch (const std::exception &ex) {
                sim::writeErrorBuffer(errorBuffer, errorBufferSize, ex.what());
                return false;
            } catch (...) {
                sim::writeErrorBuffer(errorBuffer, errorBufferSize, "unknown module start error");
                return false;
            }
        },
        [](void *moduleState) {
            try {
                auto *state = static_cast<ModuleState *>(moduleState);
                if (state != nullptr) {
                    state->client.disconnect();
                }
            } catch (const std::exception &ex) {
                std::cerr << "[module-cli] stop error: " << ex.what() << "\n";
            } catch (...) {
                std::cerr << "[module-cli] stop error: unknown\n";
            }
        },
        [](void *moduleState, const char *commandLine, bool *outKeepRunning, char *errorBuffer, std::size_t errorBufferSize) -> bool {
            try {
                auto *state = static_cast<ModuleState *>(moduleState);
                if (state == nullptr) {
                    sim::writeErrorBuffer(errorBuffer, errorBufferSize, "module state is null");
                    return false;
                }
                if (outKeepRunning != nullptr) {
                    *outKeepRunning = true;
                }
                const std::string line = trim(commandLine != nullptr ? std::string(commandLine) : std::string());
                if (line.empty()) {
                    return true;
                }
                const std::vector<std::string> tokens = splitTokens(line);
                if (tokens.empty()) {
                    return true;
                }
                const std::string &cmd = tokens[0];
                if (cmd == "help") {
                    printHelp();
                    return true;
                }
                if (cmd == "quit" || cmd == "exit") {
                    if (outKeepRunning != nullptr) {
                        *outKeepRunning = false;
                    }
                    return true;
                }
                if (cmd == "connect") {
                    if (tokens.size() < 3u) {
                        sim::writeErrorBuffer(errorBuffer, errorBufferSize, "usage: connect <host> <port>");
                        return false;
                    }
                    unsigned int parsedPort = 0u;
                    if (!sim::text::parseNumber(tokens[2], parsedPort) || parsedPort == 0u || parsedPort > 65535u) {
                        sim::writeErrorBuffer(errorBuffer, errorBufferSize, "invalid backend port");
                        return false;
                    }
                    state->host = tokens[1];
                    state->port = static_cast<std::uint16_t>(parsedPort);
                    state->client.disconnect();
                    if (!state->client.connect(state->host, state->port)) {
                        sim::writeErrorBuffer(errorBuffer, errorBufferSize, "connect failed");
                        return false;
                    }
                    std::cout << "[module-cli] connected to " << state->host << ":" << state->port << "\n";
                    return true;
                }
                if (cmd == "reconnect") {
                    state->client.disconnect();
                    if (!state->client.connect(state->host, state->port)) {
                        sim::writeErrorBuffer(errorBuffer, errorBufferSize, "reconnect failed");
                        return false;
                    }
                    std::cout << "[module-cli] reconnected to " << state->host << ":" << state->port << "\n";
                    return true;
                }
                if (cmd == "status") {
                    return commandStatus(*state, errorBuffer, errorBufferSize);
                }
                if (cmd == "step") {
                    return commandStep(*state, tokens, errorBuffer, errorBufferSize);
                }
                if (cmd == "pause") {
                    return sendSimpleCommand(*state, std::string(sim::protocol::cmd::Pause), errorBuffer, errorBufferSize);
                }
                if (cmd == "resume") {
                    return sendSimpleCommand(*state, std::string(sim::protocol::cmd::Resume), errorBuffer, errorBufferSize);
                }
                if (cmd == "toggle") {
                    return sendSimpleCommand(*state, std::string(sim::protocol::cmd::Toggle), errorBuffer, errorBufferSize);
                }
                if (cmd == "reset") {
                    return sendSimpleCommand(*state, std::string(sim::protocol::cmd::Reset), errorBuffer, errorBufferSize);
                }
                if (cmd == "recover") {
                    return sendSimpleCommand(*state, std::string(sim::protocol::cmd::Recover), errorBuffer, errorBufferSize);
                }
                if (cmd == "shutdown") {
                    return sendSimpleCommand(*state, std::string(sim::protocol::cmd::Shutdown), errorBuffer, errorBufferSize);
                }

                sim::writeErrorBuffer(errorBuffer, errorBufferSize, "unknown module command");
                return false;
            } catch (const std::exception &ex) {
                sim::writeErrorBuffer(errorBuffer, errorBufferSize, ex.what());
                return false;
            } catch (...) {
                sim::writeErrorBuffer(errorBuffer, errorBufferSize, "unknown module command error");
                return false;
            }
        }
    };
    return &exports;
}
