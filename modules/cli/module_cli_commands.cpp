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
        std::string_view commandLine,
        const grav_module::FrontendModuleCommandControl &commandControl,
        const grav_frontend::ErrorBufferView &errorBuffer)
    {
        try {
            commandControl.setContinue();
            const std::string line = ModuleCliText::trim(std::string(commandLine));
            if (line.empty()) {
                return true;
            }
            const std::vector<std::string> tokens = ModuleCliText::splitTokens(line);
            if (tokens.empty()) {
                return true;
            }
            return dispatch(state, tokens, commandControl, errorBuffer);
        } catch (const std::exception &ex) {
            errorBuffer.write(ex.what());
            return false;
        } catch (...) {
            errorBuffer.write("unknown module command error");
            return false;
        }
    }

private:
    static bool dispatch(
        ModuleState &state,
        const std::vector<std::string> &tokens,
        const grav_module::FrontendModuleCommandControl &commandControl,
        const grav_frontend::ErrorBufferView &errorBuffer)
    {
        const std::string &cmd = tokens[0];
        if (cmd == "help") {
            printHelp();
            return true;
        }
        if (cmd == "quit" || cmd == "exit") {
            commandControl.requestStop();
            return true;
        }
        if (cmd == "connect") {
            return ModuleCliBackendOps::connect(state, tokens, errorBuffer);
        }
        if (cmd == "reconnect") {
            return ModuleCliBackendOps::reconnect(state, errorBuffer);
        }
        if (cmd == "status") {
            return ModuleCliBackendOps::commandStatus(state, errorBuffer);
        }
        if (cmd == "step") {
            return ModuleCliBackendOps::commandStep(state, tokens, errorBuffer);
        }
        if (cmd == "pause") {
            return ModuleCliBackendOps::sendSimpleCommand(state, std::string(grav_protocol::Pause), errorBuffer);
        }
        if (cmd == "resume") {
            return ModuleCliBackendOps::sendSimpleCommand(state, std::string(grav_protocol::Resume), errorBuffer);
        }
        if (cmd == "toggle") {
            return ModuleCliBackendOps::sendSimpleCommand(state, std::string(grav_protocol::Toggle), errorBuffer);
        }
        if (cmd == "reset") {
            return ModuleCliBackendOps::sendSimpleCommand(state, std::string(grav_protocol::Reset), errorBuffer);
        }
        if (cmd == "recover") {
            return ModuleCliBackendOps::sendSimpleCommand(state, std::string(grav_protocol::Recover), errorBuffer);
        }
        if (cmd == "shutdown") {
            return ModuleCliBackendOps::sendSimpleCommand(state, std::string(grav_protocol::Shutdown), errorBuffer);
        }
        errorBuffer.write("unknown module command");
        return false;
    }
};

void ModuleCliCommands::printHelp()
{
    ModuleCliCommandsLocal::printHelp();
}

bool ModuleCliCommands::handleCommand(
    ModuleState &state,
    std::string_view commandLine,
    const grav_module::FrontendModuleCommandControl &commandControl,
    const grav_frontend::ErrorBufferView &errorBuffer)
{
    return ModuleCliCommandsLocal::handleCommand(state, commandLine, commandControl, errorBuffer);
}

} // namespace grav_module_cli
