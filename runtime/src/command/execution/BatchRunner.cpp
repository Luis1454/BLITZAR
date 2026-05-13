/*
 * @file runtime/src/command/execution/BatchRunner.cpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Runtime implementation for protocol, command, client, and FFI boundaries.
 */

#include "command/execution/BatchRunner.hpp"
#include "command/execution/Executor.hpp"
#include "command/parsing/Parser.hpp"
#include <fstream>
#include <sstream>

namespace bltzr_cmd {
Result runScriptFile(const std::string& path, ExecutionContext& context)
{
    std::ifstream in(path);
    if (!in.is_open()) {
        Result result{};
        result.ok = false;
        result.message = "failed to open script: " + path;
        return result;
    }
    std::ostringstream buffer;
    buffer << in.rdbuf();
    const ParseResult parsed = parseScript(buffer.str());
    if (!parsed.ok) {
        Result result{};
        result.ok = false;
        result.message = parsed.error;
        return result;
    }
    for (const Request& request : parsed.requests) {
        const Result stepResult = execute(request, context);
        if (!stepResult.ok) {
            Result result{};
            result.ok = false;
            result.message =
                "line " + std::to_string(request.lineNumber) + ": " + stepResult.message;
            return result;
        }
    }
    return Result{true, {}};
}
} // namespace bltzr_cmd
