/*
 * @file runtime/include/command/CommandExecutor.hpp
 * @author Luis1454
 * @project BLITZAR
 * @brief Runtime public interfaces for protocol, command, client, and FFI boundaries.
 */

#ifndef BLITZAR_RUNTIME_INCLUDE_COMMAND_COMMANDEXECUTOR_HPP_
#define BLITZAR_RUNTIME_INCLUDE_COMMAND_COMMANDEXECUTOR_HPP_
#include "command/CommandContext.hpp"
#include "command/CommandTypes.hpp"

namespace bltzr_cmd {
class CommandExecutor final {
public:
    static CommandResult execute(const CommandRequest& request, CommandExecutionContext& context);
};
} // namespace bltzr_cmd
#endif // BLITZAR_RUNTIME_INCLUDE_COMMAND_COMMANDEXECUTOR_HPP_
