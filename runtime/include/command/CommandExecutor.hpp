// File: runtime/include/command/CommandExecutor.hpp
// Purpose: Runtime integration surface for BLITZAR clients and protocols.

#ifndef GRAVITY_RUNTIME_INCLUDE_COMMAND_COMMANDEXECUTOR_HPP_
#define GRAVITY_RUNTIME_INCLUDE_COMMAND_COMMANDEXECUTOR_HPP_
#include "command/CommandContext.hpp"
#include "command/CommandTypes.hpp"

namespace grav_cmd {
/// Description: Defines the CommandExecutor data or behavior contract.
class CommandExecutor final {
public:
    /// Description: Describes the execute operation contract.
    static CommandResult execute(const CommandRequest& request, CommandExecutionContext& context);
};
} // namespace grav_cmd
#endif // GRAVITY_RUNTIME_INCLUDE_COMMAND_COMMANDEXECUTOR_HPP_
