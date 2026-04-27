// File: runtime/include/command/CommandBatchRunner.hpp
// Purpose: Runtime integration surface for BLITZAR clients and protocols.

#ifndef GRAVITY_RUNTIME_INCLUDE_COMMAND_COMMANDBATCHRUNNER_HPP_
#define GRAVITY_RUNTIME_INCLUDE_COMMAND_COMMANDBATCHRUNNER_HPP_
#include "command/CommandContext.hpp"
#include "command/CommandTypes.hpp"
#include <string>
namespace grav_cmd {
/// Description: Defines the CommandBatchRunner data or behavior contract.
class CommandBatchRunner final {
public:
    /// Description: Executes the runScriptFile operation.
    static CommandResult runScriptFile(const std::string& path, CommandExecutionContext& context);
};
} // namespace grav_cmd
#endif // GRAVITY_RUNTIME_INCLUDE_COMMAND_COMMANDBATCHRUNNER_HPP_
