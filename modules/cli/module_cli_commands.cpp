#include <cstddef>
#include <exception>
#include <iostream>
#include <string>
#include <vector>

#include "frontend/ErrorBuffer.hpp"
#include "protocol/BackendProtocol.hpp"
#include "modules/cli/module_cli_backend_ops.hpp"
#include "modules/cli/module_cli_commands.hpp"
#include "modules/cli/module_cli_text.hpp"

namespace grav_module_cli {

class ModuleCliCommandsLocal final {
public:
    static void printHelp()
    {
        std::cout << "[module-cli] commands:\n"
                  << "  help\n  connect <host> <port>\n  reconnect\n  status\n"
                  << "  pause | resume | toggle\n  step [count]\n  reset | recover\n  shutdown\n";
    }

    static bool handleCommand(
        ModuleState &state,
        const char *commandLine,
        bool *outKeepRunning,
        char *errorBuffer,
        std::size_t errorBufferSize)
    {
        try {
            if (outKeepRunning != nullptr) {
                *outKeepRunning = true;
            }
            const std::string line = ModuleCliText::trim(commandLine != nullptr ? std::string(commandLine) : std::string());
            if (line.empty()) {
                return true;
            }
            const std::vector<std::string> tokens = ModuleCliText::splitTokens(line);
            if (tokens.empty()) {
                return true;
            }
            return dispatch(state, tokens, outKeepRunning, errorBuffer, errorBufferSize);
        } catch (const std::exception &ex) {
            grav_frontend::writeErrorBuffer(errorBuffer, errorBufferSize, ex.what());
            return false;
        } catch (...) {
            grav_frontend::writeErrorBuffer(errorBuffer, errorBufferSize, "unknown module command error");
            return false;
        }
    }

private:
    static bool dispatch(
        ModuleState &state,
        const std::vector<std::string> &tokens,
        bool *outKeepRunning,
        char *errorBuffer,
        std::size_t errorBufferSize)
    {
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
            return ModuleCliBackendOps::connect(state, tokens, errorBuffer, errorBufferSize);
        }
        if (cmd == "reconnect") {
            return ModuleCliBackendOps::reconnect(state, errorBuffer, errorBufferSize);
        }
        if (cmd == "status") {
            return ModuleCliBackendOps::commandStatus(state, errorBuffer, errorBufferSize);
        }
        if (cmd == "step") {
            return ModuleCliBackendOps::commandStep(state, tokens, errorBuffer, errorBufferSize);
        }
        if (cmd == "pause") {
            return ModuleCliBackendOps::sendSimpleCommand(state, std::string(grav_protocol::Pause), errorBuffer, errorBufferSize);
        }
        if (cmd == "resume") {
            return ModuleCliBackendOps::sendSimpleCommand(state, std::string(grav_protocol::Resume), errorBuffer, errorBufferSize);
        }
        if (cmd == "toggle") {
            return ModuleCliBackendOps::sendSimpleCommand(state, std::string(grav_protocol::Toggle), errorBuffer, errorBufferSize);
        }
        if (cmd == "reset") {
            return ModuleCliBackendOps::sendSimpleCommand(state, std::string(grav_protocol::Reset), errorBuffer, errorBufferSize);
        }
        if (cmd == "recover") {
            return ModuleCliBackendOps::sendSimpleCommand(state, std::string(grav_protocol::Recover), errorBuffer, errorBufferSize);
        }
        if (cmd == "shutdown") {
            return ModuleCliBackendOps::sendSimpleCommand(state, std::string(grav_protocol::Shutdown), errorBuffer, errorBufferSize);
        }
        grav_frontend::writeErrorBuffer(errorBuffer, errorBufferSize, "unknown module command");
        return false;
    }
};

void ModuleCliCommands::printHelp()
{
    ModuleCliCommandsLocal::printHelp();
}

bool ModuleCliCommands::handleCommand(
    ModuleState &state,
    const char *commandLine,
    bool *outKeepRunning,
    char *errorBuffer,
    std::size_t errorBufferSize)
{
    return ModuleCliCommandsLocal::handleCommand(state, commandLine, outKeepRunning, errorBuffer, errorBufferSize);
}

} // namespace grav_module_cli
