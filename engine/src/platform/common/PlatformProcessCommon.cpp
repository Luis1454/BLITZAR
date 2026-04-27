// File: engine/src/platform/common/PlatformProcessCommon.cpp
// Purpose: Engine implementation for the BLITZAR simulation core.

#include "platform/common/PlatformProcessCommon.hpp"
namespace grav_platform {
/// Description: Executes the quoteProcessArg operation.
std::string quoteProcessArg(const std::string& arg)
{
    if (arg.empty()) {
        return "\"\"";
    }
    const bool needsQuotes = arg.find_first_of(" \t\"") != std::string::npos;
    if (!needsQuotes) {
        return arg;
    }
    std::string escaped;
    escaped.reserve(arg.size() + 8u);
    for (char c : arg) {
        if (c == '"') {
            escaped += "\\\"";
        } else {
            escaped.push_back(c);
        }
    }
    return "\"" + escaped + "\"";
}
std::string buildProcessCommandLine(const std::string& executable,
                                    const std::vector<std::string>& args)
{
    std::string command = quoteProcessArg(executable);
    for (const std::string& arg : args) {
        command.push_back(' ');
        command += quoteProcessArg(arg);
    }
    return command;
}
} // namespace grav_platform
