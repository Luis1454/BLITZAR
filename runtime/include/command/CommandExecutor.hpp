#ifndef GRAVITY_RUNTIME_INCLUDE_COMMAND_COMMANDEXECUTOR_HPP_
#define GRAVITY_RUNTIME_INCLUDE_COMMAND_COMMANDEXECUTOR_HPP_
#include "command/CommandContext.hpp"
#include "command/CommandTypes.hpp"
namespace grav_cmd {
class CommandExecutor final {
public:
    static CommandResult execute(const CommandRequest& request, CommandExecutionContext& context);
};
} // namespace grav_cmd
#endif // GRAVITY_RUNTIME_INCLUDE_COMMAND_COMMANDEXECUTOR_HPP_
