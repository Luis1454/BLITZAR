/*
 * @file modules/cli/Commands.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Command-line client module for runtime control workflows.
 */

#include "modules/cli/Commands.hpp"
#include "client/diagnostics/ErrorBuffer.hpp"
#include "command/catalog/Catalog.hpp"
#include "command/core/Context.hpp"
#include "command/execution/Executor.hpp"
#include "command/parsing/Parser.hpp"
#include <cstddef>
#include <exception>
#include <iostream>
#include <string>

namespace bltzr_module_cli {
namespace {
void printHelpImpl()
{
    std::cout << "[module-cli] commands:\n"
              << bltzr_cmd::Catalog::renderHelp() << "  quit\n"
              << "  exit\n";
}

bool handleCommandImpl(State& state, std::string_view commandLine,
                       const bltzr_module::CommandControl& commandControl,
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
        const bltzr_cmd::ParseResult parsed = bltzr_cmd::parseLine(line, 1u);
        if (!parsed.ok) {
            errorBuffer.write(parsed.error);
            return false;
        }
        if (parsed.requests.empty()) {
            return true;
        }
        bltzr_cmd::ExecutionContext context{state.transport, state.session,
                                            bltzr_cmd::ExecutionMode::Interactive, std::cout};
        const bltzr_cmd::Result result =
            bltzr_cmd::execute(parsed.requests.front(), context);
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
} // namespace

void Commands::printHelp()
{
    printHelpImpl();
}

bool Commands::handleCommand(
    State& state, std::string_view commandLine,
    const bltzr_module::CommandControl& commandControl,
    const bltzr_client::ErrorBufferView& errorBuffer)
{
    return handleCommandImpl(state, commandLine, commandControl, errorBuffer);
}
} // namespace bltzr_module_cli
