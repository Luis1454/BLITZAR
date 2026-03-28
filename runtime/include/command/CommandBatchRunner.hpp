#ifndef GRAVITY_RUNTIME_INCLUDE_COMMAND_COMMANDBATCHRUNNER_HPP_
#define GRAVITY_RUNTIME_INCLUDE_COMMAND_COMMANDBATCHRUNNER_HPP_
#include "command/CommandContext.hpp"
#include "command/CommandTypes.hpp"
#include <string>
namespace grav_cmd {
class CommandBatchRunner final {
public:
    static CommandResult runScriptFile(const std::string& path, CommandExecutionContext& context);
};
} // namespace grav_cmd
#endif // GRAVITY_RUNTIME_INCLUDE_COMMAND_COMMANDBATCHRUNNER_HPP_
