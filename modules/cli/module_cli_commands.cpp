/*
 * @file modules/cli/module_cli_commands.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Command-line client module for runtime control workflows.
 */

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

namespace bltzr_module_cli {
class ModuleCliCommandsLocal final {
public:
    static void printHelp()
    {
        std::cout << "[module-cli] commands:\n"
                  << bltzr_cmd::CommandCatalog::renderHelp() << "  quit\n"
                  << "  exit\n";
    }

    static bool handleCommand(ModuleState& state, std::string_view commandLine,
                              const bltzr_module::ClientModuleCommandControl& commandControl,
                              const bltzr_client::ErrorBufferView& errorBuffer)
    {
        try {
            commandControl.setContinue();
            const std::string line(commandLine);
            if (line.empty()) {
                return true;
            }
            if (line == "quit" || line == "exit") {
                commandControl.requestStop();
                return true;
            }
            const bltzr_cmd::CommandParseResult parsed =
                bltzr_cmd::CommandParser::parseLine(line, 1u);
            if (!parsed.ok) {
                errorBuffer.write(parsed.error);
                return false;
            }
            if (parsed.requests.empty()) {
                return true;
            }
            bltzr_cmd::CommandExecutionContext context{state.transport, state.session,
                                                       bltzr_cmd::CommandExecutionMode::Interactive,
                                                       std::cout};
            const bltzr_cmd::CommandResult result =
                bltzr_cmd::CommandExecutor::execute(parsed.requests.front(), context);
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

void ModuleCliCommands::printHelp()
{
    ModuleCliCommandsLocal::printHelp();
}

bool ModuleCliCommands::handleCommand(
    ModuleState& state, std::string_view commandLine,
    const bltzr_module::ClientModuleCommandControl& commandControl,
    const bltzr_client::ErrorBufferView& errorBuffer)
{
    return ModuleCliCommandsLocal::handleCommand(state, commandLine, commandControl, errorBuffer);
}
} // namespace bltzr_module_cli
