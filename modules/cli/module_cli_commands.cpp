// File: modules/cli/module_cli_commands.cpp
// Purpose: Client module implementation for BLITZAR extension workflows.

#include "modules/cli/module_cli_commands.hpp"
#include "client/ErrorBuffer.hpp"
#include "command/CommandCatalog.hpp"
#include "command/CommandContext.hpp"
#include "command/CommandExecutor.hpp"
#include "command/CommandParser.hpp"
#include <cstddef>
#include <exception>
#include <iostream>
#include <string>
namespace grav_module_cli {
/// Description: Defines the ModuleCliCommandsLocal data or behavior contract.
class ModuleCliCommandsLocal final {
public:
    /// Description: Executes the printHelp operation.
    static void printHelp()
    {
        std::cout << "[module-cli] commands:\n"
                  << grav_cmd::CommandCatalog::renderHelp() << "  quit\n"
                  << "  exit\n";
    }
    static bool handleCommand(ModuleState& state, std::string_view commandLine,
                              const grav_module::ClientModuleCommandControl& commandControl,
                              const grav_client::ErrorBufferView& errorBuffer)
    {
        try {
            commandControl.setContinue();
            /// Description: Executes the line operation.
            const std::string line(commandLine);
            if (line.empty()) {
                return true;
            }
            if (line == "quit" || line == "exit") {
                commandControl.requestStop();
                return true;
            }
            const grav_cmd::CommandParseResult parsed =
                /// Description: Executes the parseLine operation.
                grav_cmd::CommandParser::parseLine(line, 1u);
            if (!parsed.ok) {
                errorBuffer.write(parsed.error);
                return false;
            }
            if (parsed.requests.empty()) {
                return true;
            }
            grav_cmd::CommandExecutionContext context{state.transport, state.session,
                                                      grav_cmd::CommandExecutionMode::Interactive,
                                                      std::cout};
            const grav_cmd::CommandResult result =
                /// Description: Executes the execute operation.
                grav_cmd::CommandExecutor::execute(parsed.requests.front(), context);
            if (!result.ok) {
                errorBuffer.write(result.message);
                return false;
            }
            return true;
        }
        catch (const std::exception& ex) {
            errorBuffer.write(ex.what());
            return false;
        }
        catch (...) {
            errorBuffer.write("unknown module command error");
            return false;
        }
    }
};
/// Description: Executes the printHelp operation.
void ModuleCliCommands::printHelp()
{
    /// Description: Executes the printHelp operation.
    ModuleCliCommandsLocal::printHelp();
}
bool ModuleCliCommands::handleCommand(ModuleState& state, std::string_view commandLine,
                                      const grav_module::ClientModuleCommandControl& commandControl,
                                      const grav_client::ErrorBufferView& errorBuffer)
{
    return ModuleCliCommandsLocal::handleCommand(state, commandLine, commandControl, errorBuffer);
}
} // namespace grav_module_cli
