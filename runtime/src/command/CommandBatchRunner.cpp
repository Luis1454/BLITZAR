/*
 * @file runtime/src/command/CommandBatchRunner.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Runtime implementation for protocol, command, client, and FFI boundaries.
 */

#include "command/CommandBatchRunner.hpp"
#include "command/CommandExecutor.hpp"
#include "command/CommandParser.hpp"
#include <fstream>
#include <sstream>

namespace grav_cmd {
CommandResult CommandBatchRunner::runScriptFile(const std::string& path,
                                                CommandExecutionContext& context)
{
    std::ifstream in(path);
    if (!in.is_open()) {
        CommandResult result{};
        result.ok = false;
        result.message = "failed to open script: " + path;
        return result;
    }
    std::ostringstream buffer;
    buffer << in.rdbuf();
    const CommandParseResult parsed = CommandParser::parseScript(buffer.str());
    if (!parsed.ok) {
        CommandResult result{};
        result.ok = false;
        result.message = parsed.error;
        return result;
    }
    for (const CommandRequest& request : parsed.requests) {
        const CommandResult stepResult = CommandExecutor::execute(request, context);
        if (!stepResult.ok) {
            CommandResult result{};
            result.ok = false;
            result.message =
                "line " + std::to_string(request.lineNumber) + ": " + stepResult.message;
            return result;
        }
    }
    return CommandResult{true, {}};
}
} // namespace grav_cmd
