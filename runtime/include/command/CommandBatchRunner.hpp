/*
 * @file runtime/include/command/CommandBatchRunner.hpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Runtime public interfaces for protocol, command, client, and FFI boundaries.
 */

#ifndef BLITZAR_RUNTIME_INCLUDE_COMMAND_COMMANDBATCHRUNNER_HPP_
#define BLITZAR_RUNTIME_INCLUDE_COMMAND_COMMANDBATCHRUNNER_HPP_
#include "command/CommandContext.hpp"
#include "command/CommandTypes.hpp"
#include <string>

namespace bltzr_cmd {
class CommandBatchRunner final {
public:
    static CommandResult runScriptFile(const std::string& path, CommandExecutionContext& context);
};
} // namespace bltzr_cmd
#endif // BLITZAR_RUNTIME_INCLUDE_COMMAND_COMMANDBATCHRUNNER_HPP_
