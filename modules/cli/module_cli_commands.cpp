#include <cstddef>
#include <exception>
#include <iostream>
#include <string>
#include <vector>

#include "client/ErrorBuffer.hpp"
#include "protocol/ServerProtocol.hpp"
#include "modules/cli/module_cli_server_ops.hpp"
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
        const grav_module::ClientModuleCommandControl &commandControl,
        const grav_client::ErrorBufferView &errorBuffer)
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
        const grav_module::ClientModuleCommandControl &commandControl,
        const grav_client::ErrorBufferView &errorBuffer)
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
            return ModuleCliServerOps::connect(state, tokens, errorBuffer);
        }
        if (cmd == "reconnect") {
            return ModuleCliServerOps::reconnect(state, errorBuffer);
        }
        if (cmd == "status") {
            return ModuleCliServerOps::commandStatus(state, errorBuffer);
        }
        if (cmd == "step") {
            return ModuleCliServerOps::commandStep(state, tokens, errorBuffer);
        }
        if (cmd == "pause") {
            return ModuleCliServerOps::sendSimpleCommand(state, std::string(grav_protocol::Pause), errorBuffer);
        }
        if (cmd == "resume") {
            return ModuleCliServerOps::sendSimpleCommand(state, std::string(grav_protocol::Resume), errorBuffer);
        }
        if (cmd == "toggle") {
            return ModuleCliServerOps::sendSimpleCommand(state, std::string(grav_protocol::Toggle), errorBuffer);
        }
        if (cmd == "reset") {
            return ModuleCliServerOps::sendSimpleCommand(state, std::string(grav_protocol::Reset), errorBuffer);
        }
        if (cmd == "recover") {
            return ModuleCliServerOps::sendSimpleCommand(state, std::string(grav_protocol::Recover), errorBuffer);
        }
        if (cmd == "shutdown") {
            return ModuleCliServerOps::sendSimpleCommand(state, std::string(grav_protocol::Shutdown), errorBuffer);
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
    const grav_module::ClientModuleCommandControl &commandControl,
    const grav_client::ErrorBufferView &errorBuffer)
{
    return ModuleCliCommandsLocal::handleCommand(state, commandLine, commandControl, errorBuffer);
}

} // namespace grav_module_cli
